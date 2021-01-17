/**
 * ESPTeamsPresence -- A standalone Microsoft Teams presence light 
 *   based on ESP32 and RGB neopixel LEDs.
 *   https://github.com/toblum/ESPTeamsPresence
 *
 * Copyright (C) 2020 Tobias Blum <make@tobiasblum.de>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * API request handler
 */
boolean requestJsonApi(JsonDocument& doc, String url, String payload = "", size_t capacity = 0, String type = "POST", boolean sendAuth = false) {
	// WiFiClient
	WiFiClientSecure *client = new WiFiClientSecure;

	#ifndef DISABLECERTCHECK
	if (url.indexOf("graph.microsoft.com") > -1) {
		client->setCACert(rootCACertificateGraph);
	} else {
		client->setCACert(rootCACertificateLogin);
	}
	#endif

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
			String header = "Bearer " +  access_token;
			https.addHeader("Authorization", header);
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

			// Just for debugging purposes:
			// if (url.indexOf("presence") > 0) {
			// 	Serial.println(client->readString());
			// }

			// File found at server (HTTP 200, 301), or HTTP 400 with response payload
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_BAD_REQUEST) {
				// Parse JSON data
				DeserializationError error = deserializeJson(doc, *client);
				client->stop();
				delete client;
				client = NULL;
				
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
	s += "  body {padding:3.5rem}\n";
	s += "  .ml-s {margin-left:1.0rem}\n";
	s += "  .mt-s {margin-top:1.0rem}\n";
	s += "  .mt {margin-top:3.5rem}\n";
	s += "  #dialog-devicelogin {max-width:800px}\n";
	s += "</style>\n";
	s += "<script>\n";
	s += "function closeDeviceLoginModal() {\n";
	s += "  document.getElementById('dialog-devicelogin').close();\n";
	s += "}\n";
	s += "function performClearSettings() {\n";
	s += "  fetch('/api/clearSettings').then(r => r.json()).then(data => {\n";
	s += "    console.log('clearSettings', data);\n";
	s += "    document.getElementById('dialog-clearsettings').close();\n";
	s += "    document.getElementById('dialog-clearsettings-result').showModal();\n";
	s += "  });\n";
	s += "}\n";
	s += "function openDeviceLoginModal() {\n";
	s += "  fetch('/api/startDevicelogin').then(r => r.json()).then(data => {\n";
	s += "    console.log('startDevicelogin', data);\n";
	s += "    if (data && data.user_code) {\n";
	s += "      document.getElementById('btn_open').href = data.verification_uri;\n";
	s += "      document.getElementById('lbl_message').innerText = data.message;\n";
	s += "      document.getElementById('code_field').value = data.user_code;\n";
	s += "    }\n";
	s += "    document.getElementById('dialog-devicelogin').showModal();\n";
	s += "  });\n";
	s += "}\n";
	s += "</script>\n";
	s += "<title>ESP32 teams presence</title></head>\n";
	s += "<body><h2>ESP32 teams presence - v" + String(VERSION) + "</h2>";

	s += "<section class=\"mt\"><div class=\"nes-balloon from-left\">";
	if (strlen(paramTenantValue) == 0 || strlen(paramClientIdValue) == 0) {
		s += "<p class=\"note nes-text is-error\">Some settings are missing. Go to <a href=\"config\">configuration page</a> to complete setup.</p></div>";
	} else {
		if (access_token == "") {
			s += "<p class=\"note nes-text is-error\">No authentication infos found, start device login flow to complete widget setup!</p></div>";
		} else {
			s += "<p class=\"note nes-text\">Device setup complete, but you can start the device login flow if you need to re-authenticate.</p></div>";
		}
		s += "<div><button type=\"button\" class=\"nes-btn\" onclick=\"openDeviceLoginModal()\">Start device login</button></div>";
	}
	s += "<dialog class=\"nes-dialog is-rounded\" id=\"dialog-devicelogin\">\n";
	s += "<p class=\"title\">Start device login</p>\n";
	s += "<p id=\"lbl_message\"></p>\n";
	s += "<input type=\"text\" id=\"code_field\" class=\"nes-input\" disabled>\n";
	s += "<menu class=\"dialog-menu\">\n";
	s += "<button id=\"btn_close\" class=\"nes-btn\" onclick=\"closeDeviceLoginModal()\">Close</button>\n";
	s += "<a class=\"nes-btn is-primary ml-s\" id=\"btn_open\" href=\"https://microsoft.com/devicelogin\" target=\"_blank\">Open device login</a>\n";
	s += "</menu>\n";
	s += "</dialog>\n";
	s += "</section>\n";

	s += "<div class=\"nes-balloon from-left mt\">";
	s += "Go to <a href=\"config\">configuration page</a> to change settings.";
	s += "</div>";
	s += "<section class=\"nes-container with-title\"><h3 class=\"title\">Current settings</h3>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Client-ID</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramClientIdValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Tenant hostname / ID</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramTenantValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Polling interval (sec)</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramPollIntervalValue) +  "\"></div>";
	s += "<div class=\"nes-field mt-s\"><label for=\"name_field\">Number of LEDs</label><input type=\"text\" id=\"name_field\" class=\"nes-input\" disabled value=\"" + String(paramNumLedsValue) +  "\"></div>";
	s += "</section>";

	s += "<section class=\"nes-container with-title mt\"><h3 class=\"title\">Memory usage</h3>";
	s += "<div>Sketch: " + String(ESP.getFreeSketchSpace() - ESP.getSketchSize()) + " of " + String(ESP.getFreeSketchSpace()) + " bytes free</div>";
	s += "<progress class=\"nes-progress\" value=\"" + String(ESP.getSketchSize()) + "\" max=\"" + String(ESP.getFreeSketchSpace()) + "\"></progress>";
	s += "<div class=\"mt-s\">RAM: " + String(ESP.getFreeHeap()) + " of 327680 bytes free</div>";
	s += "<progress class=\"nes-progress\" value=\"" + String(327680 - ESP.getFreeHeap()) + "\" max=\"327680\"></progress>";
	s += "</section>";

	s += "<section class=\"nes-container with-title mt\"><h3 class=\"title\">Danger area</h3>";
	s += "<dialog class=\"nes-dialog is-rounded\" id=\"dialog-clearsettings\">\n";
	s += "<p class=\"title\">Really clear all settings?</p>\n";
	s += "<button class=\"nes-btn\" onclick=\"document.getElementById('dialog-clearsettings').close()\">Close</button>\n";
	s += "<button class=\"nes-btn is-error\" onclick=\"performClearSettings()\">Clear all settings</button>\n";
	s += "</dialog>\n";
	s += "<dialog class=\"nes-dialog is-rounded\" id=\"dialog-clearsettings-result\">\n";
	s += "<p class=\"title\">All settings were cleared.</p>\n";
	s += "</dialog>\n";
	s += "<div><button type=\"button\" class=\"nes-btn is-error\" onclick=\"document.getElementById('dialog-clearsettings').showModal();\">Clear all settings</button></div>";
	s += "</section>";

	s += "<div class=\"mt\"><i class=\"nes-icon github\"></i> Find the <a href=\"https://github.com/toblum/ESPTeamsPresence\" target=\"_blank\">ESPTeamsPresence</a> project on GitHub.</i></div>";

	s += "</body>\n</html>\n";

	server.send(200, "text/html", s);
}

void handleGetSettings() {
	DBG_PRINTLN("handleGetSettings()");
	
	const int capacity = JSON_OBJECT_SIZE(13);
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

    responseDoc["sketch_version"].set(VERSION);

	server.send(200, "application/json", responseDoc.as<String>());
}

// Delete EEPROM by removing the trailing sequence, remove context file
void handleClearSettings() {
	DBG_PRINTLN("handleClearSettings()");

	for (int t = 0; t < 4; t++)
	{
		EEPROM.write(t, 0);
	}
	EEPROM.commit();
	removeContext();

	server.send(200, "application/json", F("{\"action\": \"clear_settings\", \"error\": false}"));
	ESP.restart();
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
		boolean res = requestJsonApi(doc, "https://login.microsoftonline.com/" + String(paramTenantValue) + "/oauth2/v2.0/devicecode", "client_id=" + String(paramClientIdValue) + "&scope=offline_access%20openid%20Presence.Read", capacity);

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
