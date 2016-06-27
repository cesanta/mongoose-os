---
title: Remote device control
---

This example shows how to send commands to a device from the Internet.
After boot, Mongoose Firmware connects to the
[Mongoose Cloud](https://mongoose-iot.com) via secure Websocket channel.
This channel always stays alive, and either device or cloud can send
commands to each other at any time. Cloud knows the ID of the connected device.
It routes any command with device's ID as destination to this device.
See "Cloud Overview" section for more information.

- Login to [Mongoose Cloud](https://mongoose-iot.com)
- Create a new project, call it `control`
- Swith to the IDE tab
- Copy/paste the following code into the `app.js`

    ```javascript
    console.log('Hello from device control tutorial!');

    clubby.oncmd('Command1', function(data, done) {
      console.log('Received command: ', data.args);
      done();
    });
    ```

- In the IDC (Interactive Device Console), choose your target device
- Click Flash button, and wait until hello message appears in the device log
- Open a new browser tab with the account information
- Copy your Mongoose Cloud authentication token to the clipboard
- Open terminal and enter the following `curl` command:

    ```sh
      curl -d '{
        "src":"YOUR_LOGIN",
        "key": "YOUR_AUTH_TOKEN",
        "dst": "DEVICE_ID",
        "method": "Command1",
        "args": {"param1": 123}
      }' https://api.mongoose-iot.com
    ```

- Switch back to the IDE browser tab, notice the log message from the device:

  TODO(lsm): display log output
