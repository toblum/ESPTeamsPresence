# Flashing the software

If you're building the project, the easiest approach to flash the software is to use the prebuilt firmware versions, but you can easily build it on your own.

This video shows the build and setup process:  
<iframe width="560" height="315" src="https://www.youtube.com/embed/DH3zN3nLk9w" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>


## Using prebuild firmware version (recommended)

You can find the latest build on the [releases page](https://github.com/toblum/ESPTeamsPresence/releases). Download the "firmware.bin".  
> **Note:** There is also a "firmware-nocertcheck.bin". This version doesn't check the validity of the SSL certificates, when communicating with the Graph APIs. The communication is still encrypted. Use this version only if you know what that means.

### Prerequisites
Most ESP32 boards use the CP210x USB chip. If the board is not recognized by the system, you need to install the [appropriate driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers). Please find detailed instructions [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/establish-serial-connection.html).

### Step 1: Get flasher software
There are many options to flash software to the board. The [nodemcu-pyflasher](https://github.com/marcelstoer/nodemcu-pyflasher) works well and is available for Windows and MacOS.

### Step 2: Flash firmware
Plug the ESP32 board to your computer and wait for it to be detected.

Use the following settings:  
![Flasher settings](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/flasher_software.png)

- Select the previously downloaded "firmware.bin" using the "Firmware" button.
- Select the COM-port the software has detected.
- Start the flash process with the "Flash Firmware" button.

Start with a speed setting of 512000, lower the value if the flashing fails or is unstable.

The process should end with something like (numbers vary):
```
Wrote 1074928 bytes (609269 compressed) at 0x00010000 in 10.8 seconds (effective 799.9 kbit/s)...
Hash of data verified.
```

![Flasher settings](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/flash_software.gif)


## Build your own version using platform.io in VSCode

**TODO**