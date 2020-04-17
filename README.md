# ESPTeamsPresence

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
![](https://github.com/toblum/ESPTeamsPresence/workflows/BuildAndRelease/badge.svg)
![](https://img.shields.io/github/v/release/toblum/ESPTeamsPresence)

**A standalone Microsoft Teams presence light based on ESP32 and RGB neopixel LEDs.**

Find detailed informations about the project here: [https://toblum.github.io/ESPTeamsPresence/](https://toblum.github.io/ESPTeamsPresence/)

This projects implements the device login flow to authenticate against Microsoft Azure AD and to get a bearer token to call the Graph API to get presence informations for the current user. Everything is implemented in C++ code for Arduino-style microcontrollers and runs directly on the cheap and powerful wifi-connected ESP32 board. Hardware build and setup is extremely easy and consists only of two active parts and three wires and is powered via Micro-USB. It also features a cool retro-style web UI to configure the widget.


## Licence
All code is licensed under the [MPLv2 License](https://github.com/toblum/ESPTeamsPresence/blob/master/LICENSE).