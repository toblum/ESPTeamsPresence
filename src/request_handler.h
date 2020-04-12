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
		https.setConnectTimeout(10000);
		https.setTimeout(10000);

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
		return false;
    }
}


/**
 * Handle web requests 
 */

// Requests to /
void handleRoot() {
	DBG_PRINTLN("handleRoot()");
	// -- Let IotWebConf test and handle captive portal requests.
	if (iotWebConf.handleCaptivePortal()) { return; }

	String s = "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<link href=\"https://fonts.googleapis.com/css?family=Press+Start+2P\" rel=\"stylesheet\">";
	s += "<link href=\"https://unpkg.com/nes.css@2.3.0/css/nes.min.css\" rel=\"stylesheet\" />";
	s += "<style type=\"text/css\">\n";
	s += "body {padding:3.5rem}\n";
	s += ".mt-s {margin-top:1.0rem}\n";
	s += ".mt {margin-top:3.5rem}\n";
	s += "</style>\n";
	s += "<title>ESP32 teams presence</title></head>\n";
	s += "<body><h2>ESP32 teams presence</h2>";

	s += "<section class=\"mt\"><div class=\"nes-balloon from-left\">";
	if (access_token == "") {
		s += "<p class=\"note nes-text is-error\">No authentication infos found, start device login flow to complete widget setup!</p>";
	} else {
		s += "<p class=\"note nes-text\">Device setup complete, but you can start the device login flow if you need to re-authenticate.</p>";
	}
	s += "</div><div><a class=\"nes-btn\" href=\"/api/startDevicelogin\">Start device login</a></div>";
	s += "</section>";

	s += "<div class=\"nes-balloon from-left mt\">";
	s += "Go to <a href=\"config\">configuration page</a> to change settings.";
	s += "</div>";
	s += "<section class=\"nes-container with-title\"><h3 class=\"title\">Current settings</h3>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Client id</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramClientIdValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Tenant host / id</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramTenantValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Polling interval (sec)</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramPollIntervalValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Number of LEDs</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramNumLedsValue) +  "\"></div>";
	s += "</section>";

	s += "<section class=\"nes-container with-title mt\"><h3 class=\"title\">Memory usage</h3>";
	s += "<div>Sketch: " + String(ESP.getSketchSize()) + " of " + String(ESP.getFreeSketchSpace()) + " bytes free</div>";
	s += "<progress class=\"nes-progress\" value=\"" + String(ESP.getFreeSketchSpace() - ESP.getSketchSize()) + "\" max=\"" + String(ESP.getFreeSketchSpace()) + "\"></progress>";
	s += "<div class=\"mt-s\">RAM: " + String(ESP.getFreeHeap()) + " of 327680 bytes free</div>";
	s += "<progress class=\"nes-progress\" value=\"" + String(327680 - ESP.getFreeHeap()) + "\" max=\"327680\"></progress>";
	s += "</section>";

	s += "</body>\n</html>\n";

	server.send(200, "text/html", s);
}

void handleGetSettings() {
	DBG_PRINTLN("handleGetSettings()");
	
	const int capacity = JSON_OBJECT_SIZE(12);
	StaticJsonDocument<capacity> responseDoc;
	responseDoc["client_id"].set(paramClientIdValue);
	responseDoc["tenant"].set(paramTenantValue);
	responseDoc["poll_interval"].set(paramPollIntervalValue);
	responseDoc["num_leds"].set(paramNumLedsValue);

	responseDoc["heap"].set(ESP.getFreeHeap());
	responseDoc["min_heap"].set(ESP.getMinFreeHeap());
    responseDoc["sketch_size"].set(ESP.getSketchSize());
    responseDoc["free_sketch_space"].set(ESP.getFreeSketchSpace());
    responseDoc["flash_chip_size"].set(ESP.getFlashChipSize());
    responseDoc["flash_chip_speed"].set(ESP.getFlashChipSpeed());
    responseDoc["sdk_version"].set(ESP.getSdkVersion());
    responseDoc["cpu_freq"].set(ESP.getCpuFreqMHz());

	server.send(200, "application/json", responseDoc.as<String>());
}

boolean formValidator() {
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
