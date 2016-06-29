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
- Click on the user image on the top menu, open Profile in a new brower tab
- Open the terminal and copy/paste a `curl` command. Add arguments and
  prepend device ID to the destination:

    ```sh
    curl -u xxx -d '{"foo":123}' https://DEVICE_ID.api.mongoose-iot.com/Command1
    ```

- Switch back to the IDE browser tab, notice the log message from the device:

<img src="media/tut_control.png" width="100%">
