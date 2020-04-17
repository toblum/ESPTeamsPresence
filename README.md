# ESPTeamsPresence

![](https://github.com/toblum/ESPTeamsPresence/workflows/BuildAndRelease/badge.svg)


A standalone Microsoft Teams presence light based on ESP32 and RGB neopixel LEDs.

This projects implements the device login flow to authenticate against Microsoft Azure AD and to get a bearer token to call the Graph API to get presence informations for the current user. Everything is implemented in C++ code for Arduino-style microcontrollers and runs directly on the cheap and powerful wifi-connected ESP32 board. Hardware build and setup is extremely easy and consists only of two active parts and three wires and is powered via Micro-USB. It also features a cool retro-style web UI to configure the widget.