# Build the hardware

Hardware setup is extremely easy as the whole build consists of only two active parts and three wires. 

See the following schematic for the wiring:

![Schematic](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/Esp32TeamsPresence.png)

You can solder the wires directly to the neopixel ring and the ESP dev board. Or you can use dupont wires to build a non-permanent solution.

Neopixel RGB LED strips (another name for WS2811 or WS2812B LEDs) need a 5V power supply (red wire) and a ground connection (black wire). They are addressed using a single DATA wire (green) that is connected to the DATA IN connection of the neopixel.  
For this setup the DATA line should be connected to GPIO 12 (labeled as D12 on the board, see [detailed pinout](https://github.com/playelek/pinout-doit-32devkitv1)).

**Some technical background (if you're interested):**  
> The ESP 32 boards operate with voltage levels of 3.3 V, the pins are not 5V tolerant, so don't connect 5V directly to it. The neopixel strips need 5V which they can get from the Vin pin of the board that exposes the 5V from the USB power supply directly. The neopixel strips are quite power hungry, so use a USB power supply that can deliver enough current. A standard USB power supply can drive up to 16 LEDs usually without problems.  
> The GPIO pins deliver only 3.3V and the neopixels need 5V, but this works in almost all cases.  
> If you experience stability issues, you can put a 1000μF capacitor between 5V and ground to buffer voltage spikes.
> 
> For more detailed infos and advanced setups with hunderds od LEDs see the [Neopixel Uberguide](https://learn.adafruit.com/adafruit-neopixel-uberguide).


## Build instructions

### Step 1: Wire neopixels

![Neopixel connection](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/neopixel_connection.jpg)

This depends on the type of neopixels you're using. The most flexible solution is to solder three duport wires to the neopixel board. 

You need to solder three wires:
- 5V / VCC / + (mostly red or white)
- GND / Ground / - (usually black)
- DATA IN / DI / IN (any other color)
- DATA OUT / DO / OUT --> not used, only if you need to chain multiple boards


### Step 2: Connect ESP32 and neopixels

![Neopixel ESP32 connection](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/neopixel_esp32_connection.jpg)

Connect ESP32 and neopixel according to the [schematic](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/Esp32TeamsPresence.png).


### Step 3: Assemble in case

![Assemble in case](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/hardware_glue.jpg)

Assemble everything. Hot glue or blue tack helps a lot.


### Step 4: Finished

![Assemble in case](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/hardware_finished.jpg)