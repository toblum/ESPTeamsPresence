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
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <WS2812FX.h>

// Global settings
#define NUMLEDS 7  // number of LEDs on the strip
#define DATAPIN D1 // GPIO pin used to drive the LED strip
#define STATUS_PIN LED_BUILTIN					// User builtin LED for status
#define DEFAULT_POLLING_PRESENCE_INTERVAL 15	// Default interval to poll for presence info (seconds)
#define DEFAULT_ERROR_RETRY_INTERVAL 30			// Default interval to try again after errors
#define TOKEN_REFRESH_TIMEOUT 60	 			// Number of seconds until expiration before token gets refreshed
#define CONTEXT_FILE "context.json"				// Filename of the context file

#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)

// IotWebConf
// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESPTeamsPresence";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "presence";

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

// Add parameter
#define STRING_LEN 64
#define INTEGER_LEN 16
char paramClientIdValue[STRING_LEN];
char paramTenantValue[STRING_LEN];
char paramPollIntervalValue[INTEGER_LEN];
char paramNumLedsValue[INTEGER_LEN];
IotWebConfSeparator separator = IotWebConfSeparator();
IotWebConfParameter paramClientId = IotWebConfParameter("Client id", "clientId", paramClientIdValue, STRING_LEN, "text", "e.g. 12345678-1234-1234-1234-1234567890ab");
IotWebConfParameter paramTenant = IotWebConfParameter("Tenant host / id", "tenantId", paramTenantValue, STRING_LEN, "text", "e.g. contoso.onmicrosoft.com");
IotWebConfParameter paramPollInterval = IotWebConfParameter("Presence polling interval (sec)", "pollInterval", paramPollIntervalValue, INTEGER_LEN, "number", "10..300", (const char*)(DEFAULT_POLLING_PRESENCE_INTERVAL), "min='10' max='300' step='5'");
IotWebConfParameter paramNumLeds = IotWebConfParameter("Number of LEDs", "numLeds", paramNumLedsValue, INTEGER_LEN, "number", "1..500", (const char*)(NUMLEDS), "min='1' max='500' step='1'");

// HTTP client
BearSSL::WiFiClientSecure client;

// WS2812FX

WS2812FX ws2812fx = WS2812FX(NUMLEDS, DATAPIN, NEO_GRB + NEO_KHZ800);

// Global variables
String user_code = "";
String device_code = "";
uint8_t interval = 5;

String access_token = "";
String refresh_token = "";
String id_token = "";
unsigned int expires = 0;

String availability = "";
String activity = "";

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
uint8_t state = SMODEINITIAL;
uint8_t laststate = SMODEINITIAL;
static unsigned long tsPolling = 0;
uint8_t retries = 0;


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
	DBG_PRINTLN(F("saveContext() - Success"));
	// DBG_PRINTLN(contextDoc.as<String>());
}

boolean loadContext() {
	File file = SPIFFS.open(CONTEXT_FILE, "r");
	boolean success = false;

	if (!file) {
		DBG_PRINTLN(F("loadContext() - No file found"));
	} else {
		size_t size = file.size();
		if (size == 0) {
			DBG_PRINTLN(F("loadContext() - File empty"));
		} else {
			const int capacity = JSON_OBJECT_SIZE(3) + 4000;
			DynamicJsonDocument contextDoc(capacity);
			DeserializationError err = deserializeJson(contextDoc, file);

			if (err) {
				DBG_PRINT(F("loadContext() - deserializeJson() failed with code: "));
				DBG_PRINTLN(err.c_str());
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
					DBG_PRINTLN(F("loadContext() - Success"));
				} else {
					Serial.printf("loadContext() - ERROR Number of valid settings in file: %d, should be 3.\n", numSettings);
				}
				// DBG_PRINTLN(contextDoc.as<String>());
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

void setPresenceAnimation() {
	// Activity: Available, Away, BeRightBack, Busy, DoNotDisturb, InACall, InAConferenceCall, Inactive, InAMeeting, Offline, OffWork, OutOfOffice, PresenceUnknown, Presenting, UrgentInterruptionsOnly

	if (activity.equals("Available")) {
		setAnimation(0, FX_MODE_STATIC, GREEN);
	}
	if (activity.equals("Away")) {
		setAnimation(0, FX_MODE_STATIC, YELLOW);
	}
	if (activity.equals("BeRightBack")) {
		setAnimation(0, FX_MODE_STATIC, ORANGE);
	}
	if (activity.equals("Busy")) {
		setAnimation(0, FX_MODE_STATIC, PURPLE);
	}
	if (activity.equals("DoNotDisturb") || activity.equals("UrgentInterruptionsOnly")) {
		setAnimation(0, FX_MODE_STATIC, PINK);
	}
	if (activity.equals("InACall")) {
		setAnimation(0, FX_MODE_BREATH, RED);
	}
	if (activity.equals("InAConferenceCall")) {
		setAnimation(0, FX_MODE_BREATH, RED, 9000);
	}
	if (activity.equals("Inactive")) {
		setAnimation(0, FX_MODE_BREATH, WHITE);
	}
	if (activity.equals("InAMeeting")) {
		setAnimation(0, FX_MODE_SCAN, RED);
	}	
	if (activity.equals("Offline") || activity.equals("OffWork") || activity.equals("OutOfOffice") || activity.equals("PresenceUnknown")) {
		setAnimation(0, FX_MODE_STATIC, BLACK);
	}
	if (activity.equals("Presenting")) {
		setAnimation(0, FX_MODE_COLOR_WIPE, RED);
	}
}


/**
 * API request helper
 */
boolean requestJsonApi(JsonDocument& doc, String url, String payload = "", size_t capacity = 0, String type = "POST", boolean sendAuth = false) {
	// WiFiClient
	std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
	client->setInsecure();

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


/**
 * Application logic
 */

// Handler: Wifi connected
void onWifiConnected() {
	state = SMODEWIFICONNECTED;
}

// Poll for access token
void pollForToken() {
	String payload = "client_id=" + String(paramClientIdValue) + "&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=" + device_code;
	Serial.printf("pollForToken()\n");

	// const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(7) + 530; // Case 1: HTTP 400 error (not yet ready)
	const size_t capacity = JSON_OBJECT_SIZE(7) + 4090; // Case 2: Successful (bigger size of both variants, so take that one as capacity)
	DynamicJsonDocument responseDoc(capacity);
	boolean res = requestJsonApi(responseDoc, "https://login.microsoftonline.com/" + String(paramTenantValue) + "/oauth2/v2.0/token", payload, capacity);
	// DBG_PRINTLN(responseDoc.as<String>());

	if (!res) {
		state = SMODEDEVICELOGINFAILED;
	} else if (responseDoc.containsKey("error")) {
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
			unsigned int _expires_in = responseDoc["expires_in"].as<unsigned int>();
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
	DynamicJsonDocument responseDoc(capacity);
	boolean res = requestJsonApi(responseDoc, "https://graph.microsoft.com/beta/me/presence", "", capacity, "GET", true);

	if (!res) {
		state = SMODEPRESENCEREQUESTERROR;
		retries++;
	} else if (responseDoc.containsKey("error")) {
		const char* _error_code = responseDoc["error"]["code"];
		if (strcmp(_error_code, "InvalidAuthenticationToken")) {
			DBG_PRINTLN(F("pollPresence() - Refresh needed"));
			tsPolling = millis();
			state = SMODEREFRESHTOKEN;
		} else {
			Serial.printf("pollPresence() - Error: %s\n", _error_code);
			state = SMODEPRESENCEREQUESTERROR;
			retries++;
		}
	} else {
		// Store presence info
		availability = responseDoc["availability"].as<String>();
		activity = responseDoc["activity"].as<String>();
		retries = 0;

		setPresenceAnimation();
	}
}

// Refresh the access token
boolean refreshToken() {
	boolean success = false;
	// See: https://docs.microsoft.com/de-de/azure/active-directory/develop/v1-protocols-oauth-code#refreshing-the-access-tokens
	String payload = "client_id=" + String(paramClientIdValue) + "&grant_type=refresh_token&refresh_token=" + refresh_token;
	DBG_PRINTLN(F("refreshToken()"));

	const size_t capacity = JSON_OBJECT_SIZE(7) + 4120;
	DynamicJsonDocument responseDoc(capacity);
	boolean res = requestJsonApi(responseDoc, "https://login.microsoftonline.com/" + String(paramTenantValue) + "/oauth2/v2.0/token", payload, capacity);

	// Replace tokens and expiration
	if (res && responseDoc.containsKey("access_token") && responseDoc.containsKey("refresh_token")) {
		if (!responseDoc["access_token"].isNull()) {
			access_token = responseDoc["access_token"].as<String>();
			success = true;
		}
		if (!responseDoc["refresh_token"].isNull()) {
			refresh_token = responseDoc["refresh_token"].as<String>();
			success = true;
		}
		if (!responseDoc["id_token"].isNull()) {
			id_token = responseDoc["id_token"].as<String>();
		}
		if (!responseDoc["expires_in"].isNull()) {
			int _expires_in = responseDoc["expires_in"].as<unsigned int>();
			expires = millis() + (_expires_in * 1000); // Calculate timestamp when token expires
		}

		DBG_PRINTLN(F("refreshToken() - Success"));
		state = SMODEPOLLPRESENCE;
	} else {
		DBG_PRINTLN(F("refreshToken() - Error:"));
		// Set retry after timeout
		tsPolling = millis() + (DEFAULT_ERROR_RETRY_INTERVAL * 1000);
	}
	return success;
}

// Implementation of a statemachine to handle the different application states
void statemachine() {
	// Statemachine: Wifi connection start
	if (state == SMODEWIFICONNECTING && laststate != SMODEWIFICONNECTING) {
		setAnimation(0, FX_MODE_THEATER_CHASE, BLUE);
	}

	// Statemachine: After wifi is connected
	if (state == SMODEWIFICONNECTED && laststate != SMODEWIFICONNECTED)
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, GREEN);
		loadContext();
		// WiFi client
		DBG_PRINTLN(F("Wifi connected, waiting for requests ..."));
	}

	// Statemachine: Devicelogin started
	if (state == SMODEDEVICELOGINSTARTED) {
		if (laststate != SMODEDEVICELOGINSTARTED) {
			setAnimation(0, FX_MODE_THEATER_CHASE, PURPLE);
		}
		if (millis() >= tsPolling) {
			pollForToken();
			tsPolling = millis() + (interval * 1000);
		}
	}

	// Statemachine: Devicelogin failed
	if (state == SMODEDEVICELOGINFAILED) {
		DBG_PRINTLN(F("Device login failed"));
		state = SMODEWIFICONNECTED;	// Return back to initial mode
	}

	// Statemachine: Auth is ready, start polling for presence immediately
	if (state == SMODEAUTHREADY) {
		saveContext();
		state = SMODEPOLLPRESENCE;
		tsPolling = millis();
	}

	// Statemachine: Poll for presence information, even if there was a error before (handled below)
	if (state == SMODEPOLLPRESENCE) {
		if (millis() >= tsPolling) {
			DBG_PRINTLN(F("Polling presence info ..."));
			pollPresence();
			tsPolling = millis() + (atoi(paramPollIntervalValue) * 1000);
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
			setAnimation(0, FX_MODE_THEATER_CHASE, RED);
		}
		if (millis() >= tsPolling) {
			boolean success = refreshToken();
			if (success) {
				saveContext();
			}
		}
	}

	// Statemachine: Polling presence failed
	if (state == SMODEPRESENCEREQUESTERROR) {
		if (laststate != SMODEPRESENCEREQUESTERROR) {
			retries = 0;
		}
		
		Serial.printf("Polling presence failed, retry #%d.\n", retries);
		if (retries >= 5) {
			// Try token refresh
			state = SMODEREFRESHTOKEN;
		}
		state = SMODEPOLLPRESENCE;
	}

	// Update laststate
	if (laststate != state) {
		laststate = state;
		DBG_PRINTLN(F("======================================================================"));
	}
}


/**
 * Main functions
 */
void setup()
{
	Serial.begin(115200);
	DBG_PRINTLN();
	DBG_PRINTLN(F("setup() Starting up..."));
	// Serial.setDebugOutput(true);

	// WS2812FX
	ws2812fx.init();
	ws2812fx.start();
	setAnimation(0, FX_MODE_STATIC, WHITE);

	// iotWebConf - Initializing the configuration.
	iotWebConf.setStatusPin(STATUS_PIN);
	iotWebConf.setWifiConnectionTimeoutMs(5000);
	iotWebConf.addParameter(&separator);
	iotWebConf.addParameter(&paramClientId);
	iotWebConf.addParameter(&paramTenant);
	iotWebConf.addParameter(&paramPollInterval);
	iotWebConf.addParameter(&paramNumLeds);
	iotWebConf.setFormValidator(&formValidator);
	iotWebConf.getApTimeoutParameter()->visible = true;
	iotWebConf.setWifiConnectionCallback(&onWifiConnected);
	iotWebConf.setConfigSavedCallback(&onConfigSaved);
	state = SMODEWIFICONNECTING;
	iotWebConf.init();

	// WS2812FX
	ws2812fx.setLength(atoi(paramNumLedsValue));

	// HTTP server - Set up required URL handlers on the web server.
	server.on("/", handleRoot);
	server.on("/api/startDevicelogin", [] { handleStartDevicelogin(); });
	server.on("/config", [] { iotWebConf.handleConfig(); });
	server.onNotFound([]() { iotWebConf.handleNotFound(); });

	DBG_PRINTLN(F("setup() ready..."));

	// SPIFFS
	bool initok = false;
  	initok = SPIFFS.begin();
	DBG_PRINT(F("SPIFFS.begin() "));
	DBG_PRINTLN(initok);
}

void loop()
{
	// iotWebConf - doLoop should be called as frequently as possible.
	iotWebConf.doLoop();

	ws2812fx.service();

	statemachine();
}
