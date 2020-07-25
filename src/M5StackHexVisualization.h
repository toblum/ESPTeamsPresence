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
#include <M5Stack.h>
#include "FastLED.h"

#define Neopixel_PIN    26
#define NUM_LEDS    37

// WS2812FX
CRGB leds[NUM_LEDS];
uint8_t gHue = 0;
int numberLeds;


// // Multicore support for neopixel animations
// void neopixelTask(void *parameter)
// {
// 	for (;;)
// 	{
// 		ws2812fx.service();
// 		vTaskDelay(10);
// 	}
// }

// Neopixel control helper
void setAnimation(uint8_t segment, uint8_t mode, uint32_t color = RED, uint16_t speed = 3000, bool reverse = false)
{
	// uint16_t startLed, endLed = 0;

	// // Support only one segment for the moment
	// if (segment == 0)
	// {
	// 	startLed = 0;
	// 	endLed = numberLeds;
	// }
	// Serial.printf("setAnimation: %d, %d-%d, Mode: %d, Color: %d, Speed: %d\n", segment, startLed, endLed, mode, color, speed);
	// ws2812fx.setSegment(segment, startLed, endLed, mode, color, speed, reverse);
}


/**
 * The next functions have to be implemented for each visualization module
 */

// Set animations for presence visualization
void setPresenceAnimation(String availability, String activity)
{
	// Activity: Available, Away, BeRightBack, Busy, DoNotDisturb, InACall, InAConferenceCall, Inactive, InAMeeting, Offline, OffWork, OutOfOffice, PresenceUnknown, Presenting, UrgentInterruptionsOnly

	// if (activity.equals("Available"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, GREEN);
	// }
	// if (activity.equals("Away"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, YELLOW);
	// }
	// if (activity.equals("BeRightBack"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, ORANGE);
	// }
	// if (activity.equals("Busy"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, PURPLE);
	// }
	// if (activity.equals("DoNotDisturb") || activity.equals("UrgentInterruptionsOnly"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, PINK);
	// }
	// if (activity.equals("InACall"))
	// {
	// 	setAnimation(0, FX_MODE_BREATH, RED);
	// }
	// if (activity.equals("InAConferenceCall"))
	// {
	// 	setAnimation(0, FX_MODE_BREATH, RED, 9000);
	// }
	// if (activity.equals("Inactive"))
	// {
	// 	setAnimation(0, FX_MODE_BREATH, WHITE);
	// }
	// if (activity.equals("InAMeeting"))
	// {
	// 	setAnimation(0, FX_MODE_SCAN, RED);
	// }
	// if (activity.equals("Offline") || activity.equals("OffWork") || activity.equals("OutOfOffice") || activity.equals("PresenceUnknown"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, BLACK);
	// }
	// if (activity.equals("Presenting"))
	// {
	// 	setAnimation(0, FX_MODE_COLOR_WIPE, RED);
	// }
}

// Set animations for status visualization
void setStatusAnimation(String status)
{
	// if (status.equals("STARTUP"))
	// {
	// 	setAnimation(0, FX_MODE_STATIC, WHITE);
	// }
	// if (status.equals("DEVICELOGIN"))
	// {
	// 	setAnimation(0, FX_MODE_THEATER_CHASE, PURPLE);
	// }
	// if (status.equals("REFRESHTOKEN"))
	// {
	// 	setAnimation(0, FX_MODE_THEATER_CHASE, RED);
	// }
	// if (status.equals("AP_MODE"))
	// {
	// 	setAnimation(0, FX_MODE_THEATER_CHASE, WHITE);
	// }
	// if (status.equals("WIFI_CONNECTING"))
	// {
	// 	setAnimation(0, FX_MODE_THEATER_CHASE, BLUE);
	// }
	// if (status.equals("WIFI_CONNECTED"))
	// {
	// 	setAnimation(0, FX_MODE_THEATER_CHASE, GREEN);
	// }
}

// Visualization configuration was changed
void onVisualizationConfigChanged(int numLeds) {
	
}

// Initialize visualization module (called in setup())
void initVisualization(int numberLeds)
{
	Serial.println(__PRETTY_FUNCTION__);
	// ws2812fx.init();
	// rmt_tx_int(RMT_CHANNEL_0, ws2812fx.getPin());
	// ws2812fx.start();

	// ws2812fx.setLength(numberLeds);
	// ws2812fx.setCustomShow(customShow);

	M5.begin();


	FastLED.addLeds<WS2811,Neopixel_PIN,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(10);

	setStatusAnimation("STARTUP");
	
	// Pin neopixel logic to core 0
	// xTaskCreatePinnedToCore(
	// 	neopixelTask,
	// 	"Neopixels",
	// 	1000,
	// 	NULL,
	// 	1,
	// 	&TaskNeopixel,
	// 	0);
}

// Handle visualization (called in loop())
void visualizationLoop() {
	fill_rainbow( leds, NUM_LEDS, gHue, 7);
	FastLED.show();// must be executed for neopixel becoming effective
	EVERY_N_MILLISECONDS( 20 ) { gHue++; }
}