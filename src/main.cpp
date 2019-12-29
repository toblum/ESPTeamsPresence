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
const char* user_code = "";
const char* device_code = "";


/**
 * Handle web requests to "/" path.
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

void wifiConnected()
{
	wifiIsConnected = true;
}


// API Helper
DynamicJsonDocument requestJsonApi(String url, String type = "POST") {
	std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
	client->setInsecure();

	HTTPClient https;

	Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {  // HTTPS

		Serial.printf("[HTTPS] %s...\n", type.c_str());
		// start connection and send HTTP header
		int httpCode;
		if (type == "POST") {
			httpCode = https.POST("client_id=3837bbf0-30fb-47ad-bce8-f460ba9880c3&scope=offline_access%20openid");
		} else {
			httpCode = https.GET();
		}

		// httpCode will be negative on error
		if (httpCode > 0) {
			// HTTP header has been send and Server response header has been handled
			Serial.printf("[HTTPS] %s... code: %d\n", type.c_str(), httpCode);

			// file found at server
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
				Stream& responseStream = https.getStream();


				// Allocate the JSON document
				// Use arduinojson.org/v6/assistant to compute the capacity.
				const size_t capacity = JSON_OBJECT_SIZE(6) + 540;
				DynamicJsonDocument doc(capacity);
				Serial.println("Got stream");

				// Parse JSON object
				DeserializationError error = deserializeJson(doc, responseStream);
				if (error) {
					Serial.print(F("deserializeJson() failed: "));
					Serial.println(error.c_str());
				} else {
					return doc;
				}
			}
		} else {
			Serial.printf("[HTTPS] GET... failed, error: %d, %s\n", httpCode, https.errorToString(httpCode).c_str());
		}

		https.end();
    } else {
    	Serial.printf("[HTTPS] Unable to connect\n");
    }
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
	iotWebConf.init();

	// HTTP client
	client.setInsecure();

	// -- Set up required URL handlers on the web server.
	server.on("/", handleRoot);
	server.on("/config", [] { iotWebConf.handleConfig(); });
	server.onNotFound([]() { iotWebConf.handleNotFound(); });

	Serial.println("Ready.");
}

void loop()
{
	// -- doLoop should be called as frequently as possible.
	iotWebConf.doLoop();

	if (wifiIsConnected)
	{
		// WiFi client
		Serial.println("REQUEST");
		// Serial.setDebugOutput(true);

		DynamicJsonDocument doc = requestJsonApi("https://login.microsoftonline.com/***REMOVED***/oauth2/v2.0/devicecode");

		device_code = doc["device_code"];
		Serial.printf("Device code: %s\n", device_code);
		user_code = doc["user_code"];
		Serial.printf("User code: %s\n", user_code);

		wifiIsConnected = false;
	}
}
