# Buy the hardware

Getting the hardware should be quite easy as there are only two active parts involved. Both are easy to get and quite cheap.

## ESP32 (wifi-enabled microcontroller with superpowers)

The [ESP32](http://esp32.net/) is a microcontroller made by espressif that is quite powerful (up to 2 cores running with up tp 240 MHz and a ULP co-processor, 448 kB ROM and 520 kB RAM) and has many built-in features like WiFi and Bluetooth and much more. The best thing thing is that it's dirt cheap and available as handy development boards in many flavours. These dev boards have the GPIO pins exposed as external connectors to make the connection of peripherals easy. They should also have a Micro-USB connector included to make it easy to program them.

I used a [DOIT ESP32 DevKit v1](https://docs.zerynth.com/latest/official/board.zerynth.doit_esp32/docs/index.html) but any other ESP32 dev board should work also.

These boards come in different form factors and sometime with extra features. They are usually named:
- ESP32 NodeMCU 
- ESP32 DevKit C
- Lolin32
- Wemos MINI D1 ESP32
- Adafruit HUZZAH32
- Trigboard, M5 Stack, ...

Price should be below 15 € / $ for a simple board:
- https://www.amazon.de/AZDelivery-NodeMCU-Development-Nachfolgermodell-ESP8266/dp/B071P98VTG
- https://www.amazon.com/Official-Development-Bluetooth-Ultra-Low-Consumption/dp/B07VH89638

You can these even cheaper (less than 5 $) if you order directly from china:
- https://de.aliexpress.com/item/32864722159.html
- https://www.banggood.com/Geekcreit-ESP32-WiFi-bluetooth-Development-Board-Ultra-Low-Power-Consumption-Dual-Core-ESP-32-ESP-32S-p-1175488.html
- https://www.gearbest.com/boards---shields/pp_009609274324.html

DOIT ESP32 DevKit v1:  
![DOIT ESP32 DevKit v1](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/doit_esp32_board.jpg)


Wemos MINI D1 ESP32:  
![Wemos MINI D1 ESP32](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/wemos_d1_mini_esp32.jpg)

## Neopixels (WS2812B RGB LEDs)

These are individually addressable RGB LEDs that run on 5V and need only one additional wire to be addressed. They are commonly known as "Neopixels" and you can find them also as WS2812B RGB LEDs. They come in different shapes (strips, rings, bars, matrices, ...) and can easily be chained. They also don't cost much. I used a ring consisting of 16 LEDs, but you can use any form factor you like. If you use more than the 16 LEDs you maybe have to care about power delivery, see the [hardware section](build/hardware).

You can find these without problems below 10 $ and dirt ceap from china:
- https://www.amazon.de/AZDelivery-WS2812B-12-Bit-Neopixel-Arduino/dp/B07SPL2YC1
- https://www.amazon.com/NeoPixel-LED-Ring-Integrated-driver/dp/B00UW5X7PS
- https://de.aliexpress.com/item/32835427711.html

Neopixel variants:  
![Neopixel variants](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/neopixels.jpg)


## Cables

You can solder the neopixels permanently to the board with three wires (not recommended), or you can use female dupont or jumper wires to make it more flexible. But it depends on the neopixels you're using. Some have already cables attached.

You can get these really cheap:
- https://www.amazon.de/Aukru-20cm-female-female-Steckbr%C3%BCcken-Drahtbr%C3%BCcken/dp/B00OL6JZ3C
- https://de.aliexpress.com/item/32987024879.html

And you need a simple Micro-USB cable to flash the ESP32 and power it later. You definitely have one lying around somewhere. 