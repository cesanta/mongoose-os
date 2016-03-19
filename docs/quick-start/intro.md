---
title: Quick Start Guide
---

-  Download flashing utility from https://github.com/cesanta/fnc/releases
-  Download the latest version of Smart.js firmware from
   https://github.com/cesanta/smart.js/releases
-  Connect the board to your computer via the USB or serial interface
-  Start Flashnchips
-  Press `Browse`, select downloaded firmware .zip file
-  If you are using a USB connector, and "Select port" dropdown is disabled,
   then USB-to-serial driver needs to be installed:
   * FTDI drivers are at
   [FTDI website](http://www.ftdichip.com/Drivers/VCP.htm)
   * Drivers for NodeMCU v1 board is at
   [Silabs CP2102 page](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx).
-  Restart Flashnchips after driver installation. "Select port" dropdown must
   automatically pick up a serial port to where your board is connected.
-  Press "Flash Firmware" button. That will burn Smart.js firmware on
   the flash memory.
-  When burning is complete, Smart.js automatically connects a console
   to the device, prints device configuration, boot messages,
   and shows an interactive JavaScript prompt. Notice the unique
   "device_id" in the configuration - it will be needed to talk to the cloud.
   Smart.js
   ![](../../static/img/smartjs/fc2.png)
-  Two numbers shown by prompt
   are available free memory, and memory taken by Smart.js
-  Type some JavaScript expression to the console and press enter.
   Smart.js evaluates the expression and prints evaluation result:
   [<img src="../../static/img/smartjs/fc3.png" width="75%" />](../../static/img/smartjs/fc3.png)
-  Configure Wifi. This is not needed on POSIX platforms like RPI, where
   networking is already configured. Note that Smart.js provides flexible
   configuration infrastructure, described in the next section. Here,
   we use quick ad-hoc way to configure.
-  Enter `Wifi.setup('WifiNetworkName', 'WifiPassword')` to the console
-  Using your mouse, copy the value of device ID printed earlier
-  Enter `demoSendRandomData()` to start sending random numbers
   to `cloud.cesanta.com` every second, simulating real sensor data.
   `cloud.cesanta.com` however will reject that data, because it doesn't
   accept data from unregistered devices
-  Register the device on the cloud: login to
   https://cloud.cesanta.com/#/devices
-  Click on "Devices" tab, copy/paste device ID. Leave PSK field blank.
   Press "Add Device" button.
-  Switch to the "Dashboard" tab, and see real-time graph updated:
   ![](../../static/img/smartjs/dash1.png)
