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
#include <WS2812FX.h>

// WS2812FX
WS2812FX ws2812fx = WS2812FX(NUMLEDS, DATAPIN, NEO_GRB + NEO_KHZ800);
int numberLeds;

// RMT write support for neopixel animation
void customShow(void)
{
	uint8_t *pixels = ws2812fx.getPixels();
	// numBytes is one more then the size of the ws2812fx's *pixels array.
	// the extra byte is used by the driver to insert the LED reset pulse at the end.
	uint16_t numBytes = ws2812fx.getNumBytes() + 1;
	rmt_write_sample(RMT_CHANNEL_0, pixels, numBytes, false); // channel 0
}

// Multicore support for neopixel animations
void neopixelTask(void *parameter)
{
	for (;;)
	{
		ws2812fx.service();
		vTaskDelay(10);
	}
}

// Neopixel control helper
void setAnimation(uint8_t segment, uint8_t mode = FX_MODE_STATIC, uint32_t color = RED, uint16_t speed = 3000, bool reverse = false)
{
	uint16_t startLed, endLed = 0;

	// Support only one segment for the moment
	if (segment == 0)
	{
		startLed = 0;
		endLed = numberLeds;
	}
	Serial.printf("setAnimation: %d, %d-%d, Mode: %d, Color: %d, Speed: %d\n", segment, startLed, endLed, mode, color, speed);
	ws2812fx.setSegment(segment, startLed, endLed, mode, color, speed, reverse);
}


/**
 * The next functions have to be implemented for each visualization module
 */

// Set animations for presence visualization
void setPresenceAnimation(String availability, String activity)
{
	// Activity: Available, Away, BeRightBack, Busy, DoNotDisturb, InACall, InAConferenceCall, Inactive, InAMeeting, Offline, OffWork, OutOfOffice, PresenceUnknown, Presenting, UrgentInterruptionsOnly

	if (activity.equals("Available"))
	{
		setAnimation(0, FX_MODE_STATIC, GREEN);
	}
	if (activity.equals("Away"))
	{
		setAnimation(0, FX_MODE_STATIC, YELLOW);
	}
	if (activity.equals("BeRightBack"))
	{
		setAnimation(0, FX_MODE_STATIC, ORANGE);
	}
	if (activity.equals("Busy"))
	{
		setAnimation(0, FX_MODE_STATIC, PURPLE);
	}
	if (activity.equals("DoNotDisturb") || activity.equals("UrgentInterruptionsOnly"))
	{
		setAnimation(0, FX_MODE_STATIC, PINK);
	}
	if (activity.equals("InACall"))
	{
		setAnimation(0, FX_MODE_BREATH, RED);
	}
	if (activity.equals("InAConferenceCall"))
	{
		setAnimation(0, FX_MODE_BREATH, RED, 9000);
	}
	if (activity.equals("Inactive"))
	{
		setAnimation(0, FX_MODE_BREATH, WHITE);
	}
	if (activity.equals("InAMeeting"))
	{
		setAnimation(0, FX_MODE_SCAN, RED);
	}
	if (activity.equals("Offline") || activity.equals("OffWork") || activity.equals("OutOfOffice") || activity.equals("PresenceUnknown"))
	{
		setAnimation(0, FX_MODE_STATIC, BLACK);
	}
	if (activity.equals("Presenting"))
	{
		setAnimation(0, FX_MODE_COLOR_WIPE, RED);
	}
}

// Set animations for status visualization
void setStatusAnimation(String status)
{
	if (status.equals("STARTUP"))
	{
		setAnimation(0, FX_MODE_STATIC, WHITE);
	}
	if (status.equals("DEVICELOGIN"))
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, PURPLE);
	}
	if (status.equals("REFRESHTOKEN"))
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, RED);
	}
	if (status.equals("AP_MODE"))
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, WHITE);
	}
	if (status.equals("WIFI_CONNECTING"))
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, BLUE);
	}
	if (status.equals("WIFI_CONNECTED"))
	{
		setAnimation(0, FX_MODE_THEATER_CHASE, GREEN);
	}
}

// Visualization configuration was changed
void onVisualizationConfigChanged(int numLeds) {
	ws2812fx.setLength(numLeds);
}

// Initialize visualization module (called in setup())
void initVisualization(int numberLeds)
{
	Serial.println(__PRETTY_FUNCTION__);
	ws2812fx.init();
	rmt_tx_int(RMT_CHANNEL_0, ws2812fx.getPin());
	ws2812fx.start();

	ws2812fx.setLength(numberLeds);
	ws2812fx.setCustomShow(customShow);

	setStatusAnimation("STARTUP");

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

// Handle visualization (called in loop())
void visualizationLoop() {}