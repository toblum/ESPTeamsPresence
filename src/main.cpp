/**
 * IotWebConf01Minimal.ino -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf 
 *
 * Copyright (C) 2018 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/**
 * Example: Minimal
 * Description:
 *   This example will shows the bare minimum required for IotWeConf to start up.
 *   After starting up the thing, please search for WiFi access points e.g. with
 *   your phone. Use password provided in the code!
 *   After connecting to the access point the root page will automatically appears.
 *   We call this "captive portal".
 *   
 *   Please set a new password for the Thing (for the access point) as well as
 *   the SSID and password of your local WiFi. You cannot move on without these steps.
 *   
 *   You have to leave the access point before to let the Thing continue operation
 *   with connecting to configured WiFi.
 *
 *   Note that you can find detailed debug information in the serial console depending
 *   on the settings IOTWEBCONF_DEBUG_TO_SERIAL, IOTWEBCONF_DEBUG_PWD_TO_SERIAL set
 *   in the IotWebConf.h .
 */

#include <Arduino.h>
#include <IotWebConf.h>
#include <ESP8266HTTPClient.h>
// #include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <WS2812FX.h>

#define STATUS_PIN LED_BUILTIN

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESPTeamsPresence";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "aqswdefr";

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

// Add parameter
#define STRING_LEN 128
char stringParamValue[STRING_LEN];
IotWebConfParameter stringParam = IotWebConfParameter("String param", "stringParam", stringParamValue, STRING_LEN);
IotWebConfSeparator separator1 = IotWebConfSeparator();

// HTTP client
BearSSL::WiFiClientSecure client;

// WS2812FX
#define NUMLEDS 7  // number of LEDs on the strip
#define DATAPIN D1 // GPIO pin used to drive the LED strip
WS2812FX ws2812fx = WS2812FX(NUMLEDS, DATAPIN, NEO_GRB + NEO_KHZ800);

// Global variables
String user_code = "";
String device_code = "";
unsigned int interval = 5;

String access_token = "";
String refresh_token = "";
String id_token = "";
unsigned int expires = 0;

String availability = "";
String activity = "";

// Global settings
#define DEFAULT_POLLING_PRESENCE_INTERVAL 15	// Default interval to poll for presence info (seconds)
#define DEFAULT_ERROR_RETRY_INTERVAL 30			// Default interval to try again after errors
#define TOKEN_REFRESH_TIMEOUT 60	 			// Number of seconds until expiration before token gets refreshed
#define CONTEXT_FILE "context.json"				// Filename of the context file

// Statemachine
#define SMODEINITIAL 0               // Initial
#define SMODEWIFICONNECTING 1        // Wait for wifi connection
#define SMODEWIFICONNECTED 2         // Wifi connected
#define SMODEDEVICELOGINSTARTED 10   // Device login flow was started
#define SMODEDEVICELOGINFAILED 11    // Device login flow failed
#define SMODEAUTHREADY 20            // Authentication successful
#define SMODEPOLLPRESENCE 21         // Poll for presence
#define SMODEREFRESHTOKEN 22          // Access token needs refresh
#define SMODEPRESENCEREQUESTERROR 23       // Access token needs refresh
int state = SMODEINITIAL;
int laststate = SMODEINITIAL;
static unsigned long tsPolling = 0;



/**
 * Helper
 */
// Calculate token lifetime
int getTokenLifetime() {
	return (expires - millis()) / 1000;
}

// Save context information to file in SPIFFS
void saveContext() {
	const size_t capacity = JSON_OBJECT_SIZE(3);
	DynamicJsonDocument contextDoc(capacity);
	contextDoc["access_token"] = access_token.c_str();
	contextDoc["refresh_token"] = refresh_token.c_str();
	contextDoc["id_token"] = id_token.c_str();

	File contextFile = SPIFFS.open(CONTEXT_FILE, "w");
	serializeJsonPretty(contextDoc, contextFile);
	contextFile.close();
	Serial.printf("saveContext() - Success\n");
	// Serial.println(contextDoc.as<String>());
}

boolean loadContext() {
	File file = SPIFFS.open(CONTEXT_FILE, "r");
	boolean success = false;

	if (!file) {
		Serial.println("loadContext() - No file found");
	} else {
		size_t size = file.size();
		if (size == 0) {
			Serial.println("loadContext() - File empty");
		} else {
			const int capacity = JSON_OBJECT_SIZE(3) + 4000;
			DynamicJsonDocument contextDoc(capacity);
			DeserializationError err = deserializeJson(contextDoc, file);

			if (err) {
				Serial.print(F("loadContext() - deserializeJson() failed with code: "));
				Serial.println(err.c_str());
			} else {
				int numSettings = 0;
				if (!contextDoc["access_token"].isNull()) {
					access_token = contextDoc["access_token"].as<String>();
					numSettings++;
				}
				if (!contextDoc["refresh_token"].isNull()) {
					refresh_token = contextDoc["refresh_token"].as<String>();
					numSettings++;
				}
				if (!contextDoc["id_token"].isNull()){
					id_token = contextDoc["id_token"].as<String>();
					numSettings++;
				}
				if (numSettings == 3) {
					state = SMODEREFRESHTOKEN;
					success = true;
					Serial.println("loadContext() - Success");
				} else {
					Serial.printf("loadContext() - ERROR Number of valid settings in file: %d, should be 3.\n", numSettings);
				}
				// Serial.println(contextDoc.as<String>());
			}
		}
		file.close();
	}

	return success;
}


// Neopixel control
void setAnimation(uint8_t segment, uint8_t mode = FX_MODE_STATIC, uint32_t color = RED, uint16_t speed = 3000, bool reverse = false) {
	uint16_t startLed, endLed = 0;

	// Support only one segment for the moment
	if (segment == 0) {
		startLed = 0;
		endLed = NUMLEDS - 1;
	}
	Serial.printf("setAnimation: %d, %d-%d, Mode: %d, Color: %d, Speed: %d\n", segment, startLed, endLed, mode, color, speed);
	ws2812fx.setSegment(segment, startLed, endLed, mode, color, speed, reverse);
}


/**
 * API request helper
 */
JsonDocument requestJsonApi(String url, String payload = "", size_t capacity = 0, String type = "POST", boolean sendAuth = false) {
	// WiFiClient
	std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
	client->setInsecure();

	// HTTPClient
	HTTPClient https;

	// Prepare empty response
	const int emptyCapacity = JSON_OBJECT_SIZE(1);
	DynamicJsonDocument emptyDoc(emptyCapacity);

	// Serial.print("[HTTPS] begin...\n");
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
				// Serial.println(https.getString()); // Just for debug purposes
				Stream& responseStream = https.getStream();

				// Allocate the JSON document
				// Use arduinojson.org/v6/assistant to compute the capacity.
				// capacity = ESP.getFreeHeap() - 4096; // Use (almost) complete heap for testing purposes
				DynamicJsonDocument doc(capacity);

				// Parse JSON object
				DeserializationError error = deserializeJson(doc, responseStream);
				if (error) {
					Serial.print(F("deserializeJson() failed: "));
					Serial.println(error.c_str());
					Serial.println(https.getString());
					return emptyDoc;
				} else {
					return doc;
				}
			} else {
				Serial.printf("[HTTPS] Other HTTP code: %d\nResponse: ", httpCode);
				Serial.println(https.getString());
			}
		} else {
			Serial.printf("[HTTPS] Request failed: %s\n", https.errorToString(httpCode).c_str());
			return emptyDoc;
		}

		https.end();
    } else {
    	Serial.printf("[HTTPS] Unable to connect\n");
    }
	return emptyDoc;
}



/**
 * Handle web requests 
 */

// Requests to /
void handleRoot()
{
	Serial.println("handleRoot()");
	// -- Let IotWebConf test and handle captive portal requests.
	if (iotWebConf.handleCaptivePortal()) { return; }

	String s = "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>IotWebConf 01 Minimal</title></head>\n<body><h2>Hello world!</h2>";
	s += "Go to <a href=\"config\">configure page</a> to change settings.<br/>";
	s += "Start <a href=\"startDevicelogin\">device login</a> flow.";
	s += "</body>\n</html>\n";

	server.send(200, "text/html", s);
}

// Requests to /startDevicelogin
void handleStartDevicelogin() {
	// Only if not already started
	if (state != SMODEDEVICELOGINSTARTED) {
		Serial.println("handleStartDevicelogin()");

		// Request devicelogin context
		const size_t capacity = JSON_OBJECT_SIZE(6) + 540;
		DynamicJsonDocument doc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/devicecode", "client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&scope=offline_access%20openid", capacity);

		if (doc.containsKey("device_code") && doc.containsKey("user_code") && doc.containsKey("interval") && doc.containsKey("verification_uri") && doc.containsKey("message")) {
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


/**
 * Application logic
 */

// Handler: Wifi connected
void onWifiConnected() {
	state = SMODEWIFICONNECTED;
}

// Poll for access token
void pollForToken() {
	String payload = "client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=" + device_code;
	Serial.printf("pollForToken()\n");

	// const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(7) + 530; // Case 1: HTTP 400 error (not yet ready)
	const size_t capacity = JSON_OBJECT_SIZE(7) + 4090; // Case 2: Successful (bigger size of both variants, so take that one as capacity)
	DynamicJsonDocument responseDoc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/token", payload, capacity);
	// Serial.println(responseDoc.as<String>());

	if (responseDoc.containsKey("error")) {
		const char* _error = responseDoc["error"];
		const char* _error_description = responseDoc["error_description"];

		if (strcmp(_error, "authorization_pending") == 0) {
			Serial.printf("pollForToken() - Wating for authorization by user: %s\n\n", _error_description);
		} else {
			Serial.printf("pollForToken() - Unexpected error: %s, %s\n\n", _error, _error_description);
			state = SMODEDEVICELOGINFAILED;
		}
	} else {
		if (responseDoc.containsKey("access_token") && responseDoc.containsKey("refresh_token") && responseDoc.containsKey("id_token")) {
			// Save tokens and expiration
			access_token = responseDoc["access_token"].as<String>();
			refresh_token = responseDoc["refresh_token"].as<String>();
			id_token = responseDoc["id_token"].as<String>();
			int _expires_in = responseDoc["expires_in"].as<unsigned int>();
			expires = millis() + (_expires_in * 1000); // Calculate timestamp when token expires

			// Set state
			state = SMODEAUTHREADY;
		} else {
			Serial.printf("pollForToken() - Unknown response: %s\n", responseDoc.as<const char*>());
		}
	}
}

// Get presence information
void pollPresence() {
	// See: https://github.com/microsoftgraph/microsoft-graph-docs/blob/ananya/api-reference/beta/resources/presence.md
	const size_t capacity = JSON_OBJECT_SIZE(4) + 220;
	DynamicJsonDocument responseDoc = requestJsonApi("https://graph.microsoft.com/beta/me/presence", "", capacity, "GET", true);

	if (responseDoc.containsKey("error")) {
		const char* _error_code = responseDoc["error"]["code"];
		if (strcmp(_error_code, "InvalidAuthenticationToken")) {
			Serial.println("pollPresence() - Refresh needed");
			tsPolling = millis();
			state = SMODEREFRESHTOKEN;
		} else {
			Serial.printf("pollPresence() - Error: %s\n", _error_code);
			state = SMODEPRESENCEREQUESTERROR;
		}
	} else {
		// Save presence info
		availability =  responseDoc["availability"].as<String>();
		activity =  responseDoc["activity"].as<String>();
	}
}

// Refresh the access token
void refreshToken() {
	// See: https://docs.microsoft.com/de-de/azure/active-directory/develop/v1-protocols-oauth-code#refreshing-the-access-tokens
	String payload = "client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&grant_type=refresh_token&refresh_token=" + refresh_token;
	Serial.println("refreshToken()");

	const size_t capacity = JSON_OBJECT_SIZE(7) + 4120;
	JsonDocument responseDoc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/token", payload, capacity);

	// Replace tokens and expiration
	if (responseDoc.containsKey("access_token") && responseDoc.containsKey("refresh_token")) {
		if (!responseDoc["access_token"].isNull()) {
			access_token = responseDoc["access_token"].as<String>();
		}
		if (!responseDoc["refresh_token"].isNull()) {
			refresh_token = responseDoc["refresh_token"].as<String>();
		}
		if (!responseDoc["id_token"].isNull()) {
			id_token = responseDoc["id_token"].as<String>();
		}
		if (!responseDoc["expires_in"].isNull()) {
			int _expires_in = responseDoc["expires_in"].as<unsigned int>();
			expires = millis() + (_expires_in * 1000); // Calculate timestamp when token expires
		}

		Serial.println("refreshToken() - Success");
		state = SMODEPOLLPRESENCE;
	} else {
		Serial.println("refreshToken() - Error:");
		Serial.println(responseDoc.as<String>());
		// Set retry after timeout
		tsPolling = millis() + (DEFAULT_ERROR_RETRY_INTERVAL * 1000);
	}
}

// Implementation of a statemachine to handle the different application states
void statemachine() {
	// Statemachine: Wifi connection start
	if (state == SMODEWIFICONNECTING && laststate != SMODEWIFICONNECTING) {
		// setAnimation(0, FX_MODE_SCAN, BLUE);
	}

	// Statemachine: After wifi is connected
	if (state == SMODEWIFICONNECTED && laststate != SMODEWIFICONNECTED)
	{
		setAnimation(0, FX_MODE_SCAN, GREEN);
		loadContext();
		// WiFi client
		Serial.println("Wifi connected, waiting for requests ...");
	}

	// Statemachine: Devicelogin started
	if (state == SMODEDEVICELOGINSTARTED) {
		if (laststate != SMODEDEVICELOGINSTARTED) {
			setAnimation(0, FX_MODE_SCAN, PURPLE);
		}
		if (millis() >= tsPolling) {
			pollForToken();
			tsPolling = millis() + (interval * 1000);
		}
	}

	// Statemachine: Devicelogin failed
	if (state == SMODEDEVICELOGINFAILED) {
		Serial.println("Device login failed");
		state = SMODEWIFICONNECTED;	// Return back to initial mode
	}

	// Statemachine: Auth is ready, start polling for presence immediately
	if (state == SMODEAUTHREADY) {
		saveContext();
		state = SMODEPOLLPRESENCE;
		tsPolling = millis();
	}

	// Statemachine: Poll for presence information
	if (state == SMODEPOLLPRESENCE) {
		if (millis() >= tsPolling) {
			Serial.println("Polling presence info ...");
			pollPresence();
			tsPolling = millis() + (DEFAULT_POLLING_PRESENCE_INTERVAL * 1000);
			Serial.printf("--> Availability: %s, Activity: %s\n\n", availability.c_str(), activity.c_str());
		}

		if (getTokenLifetime() < TOKEN_REFRESH_TIMEOUT) {
			Serial.printf("Token needs refresh, valid for %d s.\n", getTokenLifetime());
			state = SMODEREFRESHTOKEN;
		}
	}

	// Statemachine: Refresh token
	if (state == SMODEREFRESHTOKEN) {
		if (laststate != SMODEREFRESHTOKEN) {
			setAnimation(0, FX_MODE_CHASE_FLASH, RED);
		}
		if (millis() >= tsPolling) {
			refreshToken();
			saveContext();
		}
	}

	// Update laststate
	if (laststate != state) {
		laststate = state;
		Serial.println("======================================================================");
	}
}


/**
 * Main functions
 */
void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("setup() Starting up...");
	// Serial.setDebugOutput(true);

	// WS2812FX
	ws2812fx.init();
	ws2812fx.start();
	setAnimation(0, FX_MODE_STATIC, WHITE);

	// iotWebConf - Initializing the configuration.
	iotWebConf.setStatusPin(STATUS_PIN);
	iotWebConf.setWifiConnectionTimeoutMs(5000);
	iotWebConf.addParameter(&stringParam);
	iotWebConf.addParameter(&separator1);
	iotWebConf.getApTimeoutParameter()->visible = true;
	iotWebConf.setWifiConnectionCallback(&onWifiConnected);
	state = SMODEWIFICONNECTING;
	iotWebConf.init();

	// HTTP server - Set up required URL handlers on the web server.
	server.on("/", handleRoot);
	server.on("/api/startDevicelogin", [] { handleStartDevicelogin(); });
	server.on("/config", [] { iotWebConf.handleConfig(); });
	server.onNotFound([]() { iotWebConf.handleNotFound(); });

	Serial.println("setup() ready...");

	// SPIFFS
	bool initok = false;
  	initok = SPIFFS.begin();
	Serial.print("SPIFFS.begin() ");
	Serial.println(initok);
}

void loop()
{
	// iotWebConf - doLoop should be called as frequently as possible.
	iotWebConf.doLoop();

	ws2812fx.service();

	statemachine();
}
