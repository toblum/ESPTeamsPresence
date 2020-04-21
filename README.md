# ESP32 Teams Presence Light

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
![](https://github.com/toblum/ESPTeamsPresence/workflows/BuildAndRelease/badge.svg)
![](https://img.shields.io/github/v/release/toblum/ESPTeamsPresence)


**A standalone Microsoft Teams presence light based on ESP32 and RGB neopixel LEDs.**

This project allows you to build a standalone device that visualizes your presence status from Microsoft Teams with colored LEDs. It's really easy to build and quite cheap.

See this video for a short overview:  
<iframe width="560" height="315" src="https://www.youtube.com/embed/MHl5En8YuxQ" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

Some technical details:  
This projects implements the device login flow to authenticate against Microsoft Azure AD and to get a access token. Using this token, the device can call the Microsoft Graph API to get presence information for the authenticated user. The token is automatically refreshed so that it can run standalone for some time.

Everything is implemented in C code for Arduino-style microcontrollers and runs directly on the cheap and powerful WiFi-connected ESP32 board. [Getting the hardware](https://toblum.github.io/ESPTeamsPresence/#/buy) is usually no problem. The [build and setup](https://youtu.be/DH3zN3nLk9w) procedure is extremely easy and well documented. The device consists of only two active parts and three wires and is powered via Micro-USB. It also features a cool retro-style web UI for configuration.

<iframe width="560" height="315" src="https://www.youtube.com/embed/3qcatKaqbU4" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>


## Libraries used
This project uses the following libraries from different authors:
- [IotWebConf](https://github.com/prampec/IotWebConf) by prampec
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by bblanchon
- [WS2812FX](https://github.com/kitesurfer1404/WS2812FX) by kitesurfer1404
- [Docute](https://github.com/egoist/docute) by egoist
- [NES.css](https://github.com/nostalgic-css/NES.css/) by nostalgic-css

Thanks to all the authors.


## Possible enhancements
- [ ] Make color / animation configurable via web UI.
- [ ] Make LED brightness configurable.
- [ ] Make LED PIN configurable.
- [ ] Allow manual setting of the status via UI / API.


## Licence
All code is licensed under the [MPLv2 License](https://github.com/toblum/ESPTeamsPresence/blob/master/LICENSE).