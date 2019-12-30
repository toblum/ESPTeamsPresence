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

#define STATUS_PIN LED_BUILTIN

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESPTeamsPresence";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "aqswdefr";

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
boolean wifiIsConnected = false;

// Add parameter
#define STRING_LEN 128
char stringParamValue[STRING_LEN];
IotWebConfParameter stringParam = IotWebConfParameter("String param", "stringParam", stringParamValue, STRING_LEN);
IotWebConfSeparator separator1 = IotWebConfSeparator();

// HTTP client
BearSSL::WiFiClientSecure client;


// Global variables
String user_code = "";
String device_code = "";
unsigned int interval = 5;

// Statemachine

#define SMODEINITIAL 0          // Initial
#define SMODEWIFICONNECTING 1   // Wait for wifi connection
#define SMODEWIFICONNECTED 2    // Wifi connected
#define SMODEDEVICELOGINSTARTED 10   // Device login flow was started
int state = SMODEINITIAL;
int laststate = SMODEINITIAL;
static unsigned long tsTokenPolling = 0;



void wifiConnected()
{
	wifiIsConnected = true;
	state = SMODEWIFICONNECTED;
}


// API Helper
DynamicJsonDocument requestJsonApi(String url, String payload = "", size_t capacity = 0, String type = "POST") {
	std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
	client->setInsecure();

	HTTPClient https;

	// Empty response
	const int emptyCapacity = JSON_OBJECT_SIZE(1);
	DynamicJsonDocument emptyDoc(emptyCapacity);

	Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {  // HTTPS

		Serial.printf("[HTTPS] %s ...\n", type.c_str());
		// start connection and send HTTP header
		int httpCode = 0;
		if (type == "POST") {
			httpCode = https.POST(payload);
		} else {
			httpCode = https.GET();
		}

		// httpCode will be negative on error
		if (httpCode > 0) {
			// HTTP header has been send and Server response header has been handled
			Serial.printf("[HTTPS] %s ... code: %d\n", type.c_str(), httpCode);

			// File found at server (200, 301 or 400)
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_BAD_REQUEST) {
				Stream& responseStream = https.getStream();
				Serial.printf("[HTTPS] Got HTTP stream\n");

				// Allocate the JSON document
				// Use arduinojson.org/v6/assistant to compute the capacity.
				DynamicJsonDocument doc(capacity);

				// Parse JSON object
				DeserializationError error = deserializeJson(doc, responseStream);
				if (error) {
					Serial.print(F("deserializeJson() failed: "));
					Serial.println(error.c_str());
					return emptyDoc;
				} else {
					return doc;
				}
			} else {
				Serial.print("[HTTPS] Result: ");
				Serial.println(https.getString());
			}
		} else {
			Serial.printf("[HTTPS] Request failed, HTTP error code: %d, %s\n", httpCode, https.errorToString(httpCode).c_str());
			return emptyDoc;
		}

		https.end();
    } else {
    	Serial.printf("[HTTPS] Unable to connect\n");
		return emptyDoc;
    }
}



/**
 * Handle web requests 
 */
void handleRoot()
{
	// -- Let IotWebConf test and handle captive portal requests.
	if (iotWebConf.handleCaptivePortal())
	{
		// -- Captive portal request were already served.
		return;
	}
	String s = "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>IotWebConf 01 Minimal</title></head>\n<body>Hello world!";
	s += "Go to <a href=\"config\">configure page</a> to change settings.";
	s += "</body>\n</html>\n";

	server.send(200, "text/html", s);
}

void startDevicelogin() {
	Serial.println("startDevicelogin()");
	const size_t capacity = JSON_OBJECT_SIZE(6) + 540;
	DynamicJsonDocument doc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/devicecode", "client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&scope=offline_access%20openid", capacity);

	// Save device_code and user_code
	const char* _device_code = doc["device_code"];
	const char* _user_code = doc["user_code"];
	const char* _verification_uri = doc["verification_uri"];
	const char* _message = doc["message"];

	device_code = String(_device_code);
	user_code = String(_user_code);
	interval = doc["interval"].as<unsigned int>();

	// Prepare response JSON
	const int responseCapacity = JSON_OBJECT_SIZE(3);
	StaticJsonDocument<responseCapacity> responseDoc;
	responseDoc["user_code"] = _user_code;
	responseDoc["verification_uri"] = _verification_uri;
	responseDoc["message"] = _message;

	// Serial.println(doc.as<String>());
	// Serial.println(responseDoc.as<String>());

	state = SMODEDEVICELOGINSTARTED;
	tsTokenPolling = millis() + (interval * 1000);

	// Send response
	server.send(200, "application/json", responseDoc.as<String>());
}


// Poll for token
void pollForToken() {
	Serial.printf("device_code: %s\n", device_code.c_str());
	String payload = "client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&grant_type=urn:ietf:params:oauth:grant-type:device_code&device_code=" + device_code;
	Serial.printf("Send polling request: %s\n", payload.c_str());
	const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(7) + 530;
	DynamicJsonDocument responseDoc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/token", payload, capacity);
	Serial.println(responseDoc.as<String>());
}



void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("Starting up...");

	// -- Initializing the configuration.
	iotWebConf.setStatusPin(STATUS_PIN);
	iotWebConf.setWifiConnectionTimeoutMs(5000);
	iotWebConf.addParameter(&stringParam);
	iotWebConf.addParameter(&separator1);
	iotWebConf.getApTimeoutParameter()->visible = true;
	iotWebConf.setWifiConnectionCallback(&wifiConnected);
	state = SMODEWIFICONNECTING;
	iotWebConf.init();

	// HTTP client
	client.setInsecure();

	// -- Set up required URL handlers on the web server.
	server.on("/", handleRoot);
	server.on("/startDevicelogin", [] { startDevicelogin(); });
	server.on("/config", [] { iotWebConf.handleConfig(); });
	server.onNotFound([]() { iotWebConf.handleNotFound(); });

	Serial.println("Ready.");
}

void loop()
{
	// -- doLoop should be called as frequently as possible.
	iotWebConf.doLoop();

	// After wifi is connected
	if (state == SMODEWIFICONNECTED && laststate != SMODEWIFICONNECTED)
	{
		// WiFi client
		Serial.println("Wifi connected");
		Serial.println("Waiting for requests ...");
		// Serial.setDebugOutput(true);

		wifiIsConnected = false;
	}

	// Statemachine: Devicelogin started
	if (state == SMODEDEVICELOGINSTARTED) {
		if (millis() >= tsTokenPolling) {
			Serial.println("Polling for token ...");
			pollForToken();
			tsTokenPolling = millis() + (interval * 1000);
		}
	}

	// Update laststate
	if (laststate != state) {
		laststate = state;
	}
}
