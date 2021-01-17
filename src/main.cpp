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

#include <Arduino.h>
#include <IotWebConf.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <WS2812FX.h>
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include "ESP32_RMT_Driver.h"


// Global settings
// #define NUMLEDS 16							// Number of LEDs on the strip (if not set via build flags)
// #define DATAPIN 26							// GPIO pin used to drive the LED strip (20 == GPIO/D13) (if not set via build flags)
// #define DISABLECERTCHECK 1					// Uncomment to disable https certificate checks (if not set via build flags)
// #define STATUS_PIN LED_BUILTIN				// User builtin LED for status (if not set via build flags)
#define DEFAULT_POLLING_PRESENCE_INTERVAL "30"	// Default interval to poll for presence info (seconds)
#define DEFAULT_ERROR_RETRY_INTERVAL 30			// Default interval to try again after errors
#define TOKEN_REFRESH_TIMEOUT 60	 			// Number of seconds until expiration before token gets refreshed
#define CONTEXT_FILE "/context.json"			// Filename of the context file
#define VERSION "0.17.0"						// Version of the software

#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)


#ifndef DISABLECERTCHECK
// Tool to get certs: https://projects.petrucci.ch/esp32/

// certificate for https://login.microsoftonline.com
// DigiCert Global Root CA, Valid until: 10/Nov/2031
// From: https://www.digicert.com/kb/digicert-root-certificates.htm
const char* rootCACertificateLogin = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n" \
"";


// certificate for https://graph.microsoft.com
// DigiCert Assured ID Root G2, Valid until: 15/Jan/2038
// From: https://www.digicert.com/kb/digicert-root-certificates.htm
const char* rootCACertificateGraph = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n" \
"";
#endif



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
IotWebConfParameter paramClientId = IotWebConfParameter("Client-ID (Generic ID: 3837bbf0-30fb-47ad-bce8-f460ba9880c3)", "clientId", paramClientIdValue, STRING_LEN, "text", "e.g. 3837bbf0-30fb-47ad-bce8-f460ba9880c3", "3837bbf0-30fb-47ad-bce8-f460ba9880c3");
IotWebConfParameter paramTenant = IotWebConfParameter("Tenant hostname / ID", "tenantId", paramTenantValue, STRING_LEN, "text", "e.g. contoso.onmicrosoft.com");
IotWebConfParameter paramPollInterval = IotWebConfParameter("Presence polling interval (sec) (default: 30)", "pollInterval", paramPollIntervalValue, INTEGER_LEN, "number", "10..300", DEFAULT_POLLING_PRESENCE_INTERVAL, "min='10' max='300' step='5'");
IotWebConfParameter paramNumLeds = IotWebConfParameter("Number of LEDs (default: 16)", "numLeds", paramNumLedsValue, INTEGER_LEN, "number", "1..500", "16", "min='1' max='500' step='1'");
byte lastIotWebConfState;

// HTTP client
WiFiClientSecure client;

// WS2812FX
WS2812FX ws2812fx = WS2812FX(NUMLEDS, DATAPIN, NEO_GRB + NEO_KHZ800);
int numberLeds;

// OTA update
HTTPUpdateServer httpUpdater;

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
#define SMODEREFRESHTOKEN 22         // Access token needs refresh
#define SMODEPRESENCEREQUESTERROR 23 // Access token needs refresh
uint8_t state = SMODEINITIAL;
uint8_t laststate = SMODEINITIAL;
static unsigned long tsPolling = 0;
uint8_t retries = 0;

// Multicore
TaskHandle_t TaskNeopixel; 


/**
 * Helper
 */
// Calculate token lifetime
int getTokenLifetime() {
	return (expires - millis()) / 1000;
}

// Save context information to file in SPIFFS
void saveContext() {
	const size_t capacity = JSON_OBJECT_SIZE(3) + 5000;
	DynamicJsonDocument contextDoc(capacity);
	contextDoc["access_token"] = access_token.c_str();
	contextDoc["refresh_token"] = refresh_token.c_str();
	contextDoc["id_token"] = id_token.c_str();

	File contextFile = SPIFFS.open(CONTEXT_FILE, FILE_WRITE);
	size_t bytesWritten = serializeJsonPretty(contextDoc, contextFile);
	contextFile.close();
	DBG_PRINT(F("saveContext() - Success: "));
	DBG_PRINTLN(bytesWritten);
	// DBG_PRINTLN(contextDoc.as<String>());
}

boolean loadContext() {
	File file = SPIFFS.open(CONTEXT_FILE);
	boolean success = false;

	if (!file) {
		DBG_PRINTLN(F("loadContext() - No file found"));
	} else {
		size_t size = file.size();
		if (size == 0) {
			DBG_PRINTLN(F("loadContext() - File empty"));
		} else {
			const int capacity = JSON_OBJECT_SIZE(3) + 10000;
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
					success = true;
					DBG_PRINTLN(F("loadContext() - Success"));
					if (strlen(paramClientIdValue) > 0 && strlen(paramTenantValue) > 0) {
						DBG_PRINTLN(F("loadContext() - Next: Refresh token."));
						state = SMODEREFRESHTOKEN;
					} else {
						DBG_PRINTLN(F("loadContext() - No client id or tenant setting found."));
					}
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

// Remove context information file in SPIFFS
void removeContext() {
	SPIFFS.remove(CONTEXT_FILE);
	DBG_PRINTLN(F("removeContext() - Success"));
}

void startMDNS() {
	DBG_PRINTLN("startMDNS()");
	// Set up mDNS responder
    if (!MDNS.begin(thingName)) {
        DBG_PRINTLN("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
	// MDNS.addService("http", "tcp", 80);

    DBG_PRINT("mDNS responder started: ");
    DBG_PRINT(thingName);
    DBG_PRINTLN(".local");
}


#include "request_handler.h"
#include "spiffs_webserver.h"


// Neopixel control
void setAnimation(uint8_t segment, uint8_t mode = FX_MODE_STATIC, uint32_t color = RED, uint16_t speed = 3000, bool reverse = false) {
	uint16_t startLed, endLed = 0;

	// Support only one segment for the moment
	if (segment == 0) {
		startLed = 0;
		endLed = numberLeds;
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
	const size_t capacity = JSON_OBJECT_SIZE(7) + 10000; // Case 2: Successful (bigger size of both variants, so take that one as capacity)
	DynamicJsonDocument responseDoc(capacity);
	boolean res = requestJsonApi(responseDoc, "https://login.microsoftonline.com/" + String(paramTenantValue) + "/oauth2/v2.0/token", payload, capacity);

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
	const size_t capacity = JSON_OBJECT_SIZE(4) + 500;
	DynamicJsonDocument responseDoc(capacity);
	boolean res = requestJsonApi(responseDoc, "https://graph.microsoft.com/v1.0/me/presence", "", capacity, "GET", true);

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

	const size_t capacity = JSON_OBJECT_SIZE(7) + 10000;
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

	// Statemachine: Check states of iotWebConf to detect AP mode and WiFi Connection attepmt
	byte iotWebConfState = iotWebConf.getState();
	if (iotWebConfState != lastIotWebConfState) {
		if (iotWebConfState == IOTWEBCONF_STATE_NOT_CONFIGURED || iotWebConfState == IOTWEBCONF_STATE_AP_MODE) {
			DBG_PRINTLN(F("Detected AP mode"));
			setAnimation(0, FX_MODE_THEATER_CHASE, WHITE);
		}
		if (iotWebConfState == IOTWEBCONF_STATE_CONNECTING) {
			DBG_PRINTLN(F("WiFi connecting"));
			state = SMODEWIFICONNECTING;
		}
	}
	lastIotWebConfState = iotWebConfState;

	// Statemachine: Wifi connection start
	if (state == SMODEWIFICONNECTING && laststate != SMODEWIFICONNECTING) {
		setAnimation(0, FX_MODE_THEATER_CHASE, BLUE);
	}

	// Statemachine: After wifi is connected
	if (state == SMODEWIFICONNECTED && laststate != SMODEWIFICONNECTED)
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, GREEN);
		startMDNS();
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
		} else {
			state = SMODEPOLLPRESENCE;
		}
	}

	// Update laststate
	if (laststate != state) {
		laststate = state;
		DBG_PRINTLN(F("======================================================================"));
	}
}


/**
 * Multicore
 */
void neopixelTask(void * parameter) {
	for (;;) {
		ws2812fx.service();
		vTaskDelay(10);
	}
}

void customShow(void) {
	uint8_t *pixels = ws2812fx.getPixels();
	// numBytes is one more then the size of the ws2812fx's *pixels array.
	// the extra byte is used by the driver to insert the LED reset pulse at the end.
	uint16_t numBytes = ws2812fx.getNumBytes() + 1;
	rmt_write_sample(RMT_CHANNEL_0, pixels, numBytes, false); // channel 0
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
	#ifdef DISABLECERTCHECK
		DBG_PRINTLN(F("WARNING: Checking of HTTPS certificates disabled."));
	#endif

	// WS2812FX
	ws2812fx.init();
	rmt_tx_int(RMT_CHANNEL_0, ws2812fx.getPin());
	ws2812fx.start();
	setAnimation(0, FX_MODE_STATIC, WHITE);

	// iotWebConf - Initializing the configuration.
	#ifdef LED_BUILTIN
	iotWebConf.setStatusPin(LED_BUILTIN);
	#endif
	iotWebConf.setWifiConnectionTimeoutMs(5000);
	iotWebConf.addParameter(&separator);
	iotWebConf.addParameter(&paramClientId);
	iotWebConf.addParameter(&paramTenant);
	iotWebConf.addParameter(&paramPollInterval);
	iotWebConf.addParameter(&paramNumLeds);
	// iotWebConf.setFormValidator(&formValidator);
	// iotWebConf.getApTimeoutParameter()->visible = true;
	// iotWebConf.getApTimeoutParameter()->defaultValue = "10";
	iotWebConf.setWifiConnectionCallback(&onWifiConnected);
	iotWebConf.setConfigSavedCallback(&onConfigSaved);
	iotWebConf.setupUpdateServer(&httpUpdater);
	iotWebConf.skipApStartup();
	iotWebConf.init();

	// WS2812FX
	numberLeds = atoi(paramNumLedsValue);
	if (numberLeds < 1) {
		DBG_PRINTLN(F("Number of LEDs not given, using 16."));
		numberLeds = NUMLEDS;
	}
	ws2812fx.setLength(numberLeds);
	ws2812fx.setCustomShow(customShow);

	// HTTP server - Set up required URL handlers on the web server.
	server.on("/", HTTP_GET, handleRoot);
	server.on("/config", HTTP_GET, [] { iotWebConf.handleConfig(); });
	server.on("/config", HTTP_POST, [] { iotWebConf.handleConfig(); });
	server.on("/upload", HTTP_GET, [] { handleMinimalUpload(); });
	server.on("/api/startDevicelogin", HTTP_GET, [] { handleStartDevicelogin(); });
	server.on("/api/settings", HTTP_GET, [] { handleGetSettings(); });
	server.on("/api/clearSettings", HTTP_GET, [] { handleClearSettings(); });
	server.on("/fs/delete", HTTP_DELETE, handleFileDelete);
	server.on("/fs/list", HTTP_GET, handleFileList);
	server.on("/fs/upload", HTTP_POST, []() {
		server.send(200, "text/plain", "");
	}, handleFileUpload);

	// server.onNotFound([](){ iotWebConf.handleNotFound(); });
	server.onNotFound([]() {
		iotWebConf.handleNotFound();
		if (!handleFileRead(server.uri())) {
			server.send(404, "text/plain", "FileNotFound");
		}
	});

	DBG_PRINTLN(F("setup() ready..."));

	// SPIFFS.begin() - Format if mount failed
	DBG_PRINTLN(F("SPIFFS.begin() "));
	if(!SPIFFS.begin(true)) {
		DBG_PRINTLN("SPIFFS Mount Failed");
        return;
    }

	// Pin neopixel logic to core 0
	xTaskCreatePinnedToCore(
		neopixelTask,
		"Neopixels",
		1000,
		NULL,
		1,
		&TaskNeopixel,
		0);
}

void loop()
{
	// iotWebConf - doLoop should be called as frequently as possible.
	iotWebConf.doLoop();

	statemachine();
}
