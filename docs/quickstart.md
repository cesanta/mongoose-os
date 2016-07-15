---
title: Quick Start Guide
---

- Log in to [Mongoose Cloud](https://mongoose-iot.com)
- Click on the Smart Light Example project.
  When the dialog to clone the project appears. Click "Add".
- A private clone of the Smart Light project appears. Click on it.
- Navigate to the DEVICES tab.
- If you don't have a real device,
    - Click on the "Add Device" button.
    - Select the virtual device and click "Add"
- If you do have a real device,
    - Download the
      [Mongoose Flashing Tool](https://github.com/cesanta/mft/releases).
    - Follow the Mongoose Flashing Tool wizard to flash Mongoose Firmware
      to your device and connect it to 
      [Mongoose Cloud](https://mongoose-iot.com).<br>
      <img src="media/quicktart_mft.png" width="100%">
    - Connect an LED to GND and GPIO14 pins.
    - FTDI USB drivers are available on the
    [FTDI website](http://www.ftdichip.com/Drivers/VCP.htm).
    - Drivers for NodeMCU v1 board are available on the
      [Silabs CP2102 page](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx).
- The new device will appear in the device list. Copy its ID to the clipboard.
- Navigate to the IDE tab.
- Click on `mobile_app/config.yaml` to edit the mobile configuration. Substitute
  the `MY_DEVICE_ID` with the device ID from the clipboard.
- If you're using a real device, click on the Flash button to build and flash the
  Smart Light firmware to your device.
- Install the [Mongoose Mobile app](https://play.google.com/store/apps/details?id=com.cesanta.mongoosemobile)
  from Google Play. This app will demonstrate how to control your remote device
  via the Mongoose Mobile app.
- Open the Mongoose Mobile app and login to the cloud. Use the UI to switch the LED
  on/off.
