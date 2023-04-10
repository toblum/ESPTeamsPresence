# Setup and configure

After you flashed the software, you need to setup the device to connect to your WiFi and connect to your Office 365 environment.

This video shows the build and setup process:  
<iframe width="560" height="315" src="https://www.youtube.com/embed/DH3zN3nLk9w" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

**Note:** Starting with v0.12.0 the process is simplified. "Startup delay" option is removed and proper defaults are supplied.

## Step 1: Connect to device hotspot

After you flashed your presence device, please power-cycle it once and wait for a few seconds. The device opens a own WiFi hotspot now. The name is "ESPTeamsPresence". You should be able to connect to it using your phone or any other WiFi-enabled device. The network is password-protected, use "presence" as password. It takes some seconds and you should be connected. This presence device shows a white running light animation.

![Device hotspot](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_hotspot.png)


## Step 2: Configure settings

Now open a browser on the device / phone that is connected to the hotspot and open the following URL: http://192.168.4.1/config

The configuration UI of the device should open now. It looks like this:  
![Device hotspot](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_config.png)

Please provide the following data:

- Thing name (required):  
  Name of the device and used as the name for the hotspot.  
  This name is also used as the hostname of the device. If the name is e.g. ESPTeamsPresence, you should be able to open the web interface via http://ESPTeamsPresence.local  
  Change this if you're using more than one device. 
- AP password (required):  
  Password for the device hotspot and the configuration UI. Default is "presence", change it as you like. 
- WiFi SSID (required):  
  The SSID of your local home WiFi. After setup, the device will connect to it and you will be able to reach the device locally, the device will also use your wifi to talk to the Azure cloud to get your Teams presence status.
- WiFi password (required):  
  The password of your local WiFi hotspot.


- Client-ID:  
  Client-ID of the Azure App that is used to connect to Teams. You can use the generic ID 3837bbf0-30fb-47ad-bce8-f460ba9880c3 or setup your own Azure App. The generic ID should work fine in most cases. It's a multi-tenant app that I registered. You can use it instead of registering your own Azure app (what's also possible). Find more information on how to register you own app [here](https://github.com/toblum/ESPTeamsPresence/issues/30#issuecomment-1475274011).
- Tenant hostname / ID:  
  Hostname of your tenant, e.g. contoso.onmicrosoft.com. You can also use the tenant GUID.
- Presence polling interval (sec):  
  Intervall to poll for new presence information. Don't set this too low, you may stress the Azure infrastructure too much, and get throttled. 90 seconds or more may be a good value.
- Number of LEDs:  
  The number of LEDs in neopixel ring / strip.

Click "Apply" to save everything. Disconnect from the device WiFi. Power cycle the device to restart it.

Power cycle the presence device once and wait some time. The LEDs should now:
- Show a white running animation when the device is powered on or is in AP mode for WiFi setup.
- Show a blue running animation while it's connecting to Wifi.
- Show a green running animation while it's waiting for Azure credentials.

## Step 3: Get IP / hostname of the device in your home network

Usually you should be able to reach the device via it's thingname you set up in the last step: http://\<thingname>.local/ --> e.g. http://espteamspresence.local/.  
This should work in most environments, but if not, you can reach it via it's IP address (see below).

**If you need to find out the IP address of the device:**  
While restarting, keep the serial monitor of the "ESP32 Flash Tool" open and you should see the following output and the IP the device has been assigned:  
![Device IP](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_ip.png)

Alternatively you should be able to get the IP from your routers interface or by using a app like [Fing](https://www.fing.com/products/fing-app).


## Step 4: Connect to the Azure App using device login flow

Make sure that you're connected to your home WiFi again, then open the interface in the browser, e.g. http://espteamspresence.local/ or http://192.168.0.84/ as described in the last step.

You should now see the retro-style UI of the presence device:  
![Device login](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_login_1.png)

It should say "No authentication infos found, start device login flow to complete widget setup!". Click "Start device login". The LEDs should show now a running purple animation.

A popup opens up:  
![Device login](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_login_2.png)

Copy the code that is displayed and click "Open device login" or open https://microsoft.com/devicelogin in your favorite browser. 

Enter the copied code:  
![Device login](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_login_3.png)

Login to Office 365 as usual using your regular account that you want to use for your presence.

After login is completed you should may see this dialog:  
![Device login](https://github.com/toblum/ESPTeamsPresence/raw/master/docs/pics/device_login_4.png)

It asks you to consent that the Azure App, that the presence device uses in the background, is allowed to access your profile and presence data (and nothing else).

If you have some admin role in Azure AD you may see the "Consent in the name of your organization" checkbox. If you tick this, other users in your tenant don't need to consent on their own.

Click "Accept" and you should be done. The access tokens (only the tokens, no personal credentials) are now stored on the device. The token is refreshed automatically, if needed, in the future. Everytime the token is refreshed, the device shows a running red animation for a short time.

The presence device should now receive presence information and light up in the color of your presence.
