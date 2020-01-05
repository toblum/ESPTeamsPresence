/**
 * API request handler
 */
boolean requestJsonApi(JsonDocument& doc, String url, String payload = "", size_t capacity = 0, String type = "POST", boolean sendAuth = false) {
	// WiFiClient
	WiFiClientSecure *client = new WiFiClientSecure;
	client -> setCACert(rootCACertificate);

	// HTTPClient
	HTTPClient https;

	// Prepare empty response
	const int emptyCapacity = JSON_OBJECT_SIZE(1);
	DynamicJsonDocument emptyDoc(emptyCapacity);

	// DBG_PRINT("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {  // HTTPS

		// Send auth header?
		if (sendAuth) {
			https.addHeader("Authorization", "Bearer " + access_token);
			Serial.printf("[HTTPS] Auth token valid for %d s.\n", getTokenLifetime());
		}

		// Start connection and send HTTP header
		int httpCode = 0;
		if (type == "POST") {
			httpCode = https.POST(payload);
		} else {
			httpCode = https.GET();
		}

		// httpCode will be negative on error
		if (httpCode > 0) {
			// HTTP header has been send and Server response header has been handled
			Serial.printf("[HTTPS] Method: %s, Response code: %d\n", type.c_str(), httpCode);

			// File found at server (HTTP 200, 301), or HTTP 400 with response payload
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_BAD_REQUEST) {
				// DBG_PRINTLN(https.getString()); // Just for debug purposes, breaks further execution
				Stream& responseStream = https.getStream();

				// Parse JSON data
				DeserializationError error = deserializeJson(doc, responseStream);
				if (error) {
					DBG_PRINT(F("deserializeJson() failed: "));
					DBG_PRINTLN(error.c_str());
					https.end();
					return false;
				} else {
					https.end();
					return true;
				}
			} else {
				Serial.printf("[HTTPS] Other HTTP code: %d\nResponse: ", httpCode);
				DBG_PRINTLN(https.getString());
				https.end();
				return false;
			}
		} else {
			Serial.printf("[HTTPS] Request failed: %s\n", https.errorToString(httpCode).c_str());
			https.end();
			return false;
		}
    } else {
    	DBG_PRINTLN(F("[HTTPS] Unable to connect"));
    }
	return false;
}


/**
 * Handle web requests 
 */

// Requests to /
void handleRoot()
{
	DBG_PRINTLN("handleRoot()");
	// -- Let IotWebConf test and handle captive portal requests.
	if (iotWebConf.handleCaptivePortal()) { return; }

	String s = "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>IotWebConf 01 Minimal</title></head>\n<body><h2>Hello world!</h2>";
	s += "Go to <a href=\"config\">configure page</a> to change settings.<br/><br/>";
	s += "Client id: " + String(paramClientIdValue) +  "<br/>";
	s += "Tenant host / id: " + String(paramTenantValue) +  "<br/>";
	s += "Polling interval (sec): " + String(paramPollIntervalValue) +  "<br/>";
	s += "Number of LEDs: " + String(paramNumLedsValue) +  "<br/><br/>";
	s += "Start <a href=\"/api/startDevicelogin\">device login</a> flow.";
	s += "</body>\n</html>\n";

	server.send(200, "text/html", s);
}

boolean formValidator()
{
	DBG_PRINTLN(F("Validating form."));
	boolean valid = true;

	int l1 = server.arg(paramClientId.getId()).length();
	if (l1 < 36)
	{
		paramClientId.errorMessage = "Please provide at least 36 characters for the client id!";
		valid = false;
	}

	int l2 = server.arg(paramTenant.getId()).length();
	if (l2 < 10)
	{
		paramTenant.errorMessage = "Please provide at least 10 characters for the tenant host / GUID!";
		valid = false;
	}

	int l3 = server.arg(paramPollInterval.getId()).length();
	if (l3 < 1)
	{
		paramPollInterval.errorMessage = "Please provide a value for the presence poll interval!";
		valid = false;
	}

	int l4 = server.arg(paramNumLeds.getId()).length();
	if (l4 < 1)
	{
		paramNumLeds.errorMessage = "Please provide a value for the number of LEDs!";
		valid = false;
	}

	return valid;
}

// Config was saved
void onConfigSaved() {
	DBG_PRINTLN(F("Configuration was updated."));
	ws2812fx.setLength(atoi(paramNumLedsValue));
}

// Requests to /startDevicelogin
void handleStartDevicelogin() {
	// Only if not already started
	if (state != SMODEDEVICELOGINSTARTED) {
		DBG_PRINTLN(F("handleStartDevicelogin()"));

		// Request devicelogin context
		const size_t capacity = JSON_OBJECT_SIZE(6) + 540;
		DynamicJsonDocument doc(capacity);
		boolean res = requestJsonApi(doc, "https://login.microsoftonline.com/" + String(paramTenantValue) + "/oauth2/v2.0/devicecode", "client_id=" + String(paramClientIdValue) + "&scope=offline_access%20openid", capacity);

		if (res && doc.containsKey("device_code") && doc.containsKey("user_code") && doc.containsKey("interval") && doc.containsKey("verification_uri") && doc.containsKey("message")) {
			// Save device_code, user_code and interval
			device_code = doc["device_code"].as<String>();
			user_code = doc["user_code"].as<String>();
			interval = doc["interval"].as<unsigned int>();

			// Prepare response JSON
			const size_t responseCapacity = JSON_OBJECT_SIZE(3);
			DynamicJsonDocument responseDoc(responseCapacity);
			responseDoc["user_code"] = doc["user_code"].as<const char*>();
			responseDoc["verification_uri"] = doc["verification_uri"].as<const char*>();
			responseDoc["message"] = doc["message"].as<const char*>();

			// Set state, update polling timestamp
			state = SMODEDEVICELOGINSTARTED;
			tsPolling = millis() + (interval * 1000);

			// Send JSON response
			server.send(200, "application/json", responseDoc.as<String>());
		} else {
			server.send(500, "application/json", F("{\"error\": \"devicelogin_unknown_response\"}"));
		}
	} else {
		server.send(409, "application/json", F("{\"error\": \"devicelogin_already_running\"}"));
	}
}
