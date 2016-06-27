---
title: Quick Start Guide
---

- Login to [Mongoose Cloud](https://mongoose-iot.com)
- Click on Smart Light example project.
  A dialog to clone a project appears. Click "Add".
- A private clone of the Smart Light project appears. Click on it.
- Navigate to the DEVICES tab
- If you don't have a real device,
    - Click on the "Add Device" button.
    - Select the virtual device and click Add
- If you do have a real device,
    - Download the
      [Mongoose Flashing Tool](https://github.com/cesanta/mft/releases)
    - Follow the Mongoose Flashing Tool wizard to flash Mongoose Firmware
      to your device and connect it to the
      [Mongoose Cloud](https://mongoose-iot.com)<br>
      <img src="media/quicktart_mft.png" width="100%">
    - Connect an LED to GND and GPIO5 pins
    - FTDI USB drivers are available on the
    [FTDI website](http://www.ftdichip.com/Drivers/VCP.htm)
    - Drivers for NodeMCU v1 board are available on the
      [Silabs CP2102 page](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx)
- The new device should appear in the device list. Copy it's ID to the clipboard
- Navigate to the IDE tab
- Click on `mobile_app/config.yaml` to edit the mobile configuration. Substitute
  the DEVICE_ID with the device ID from the clipboard
- If you're using a real device, click on the Flash button to build and flash
  Smart Light firmware to your device
- Find and install the Mongoose Mobile app on Google Play. This app will
  demonstrate how to control your remote device via the Mongoose Mobile app.
- Open the Mongoose Mobile app. Use the UI to switch LED on/off.
