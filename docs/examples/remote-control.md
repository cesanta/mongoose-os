---
title: Remote device control
---

This example shows how to send commands to a device from the Internet.
After booting, Mongoose Firmware connects to
[Mongoose Cloud](https://mongoose-iot.com) via a secure WebSocket channel.
This channel always stays alive, and both device and cloud can send
commands to each other at any time. The cloud knows the ID of the connected device.
It routes any command with the device's ID as destination to this device.
See "Cloud Overview" section for more information.

- Login to [Mongoose Cloud](https://mongoose-iot.com).
- Create a new project, call it `control`.
- Switch to the IDE tab.
- Copy/paste the following code into the `app.js`

    ```javascript
    console.log('Hello from device control tutorial!');

    clubby.oncmd('/Command1', function(data) {
      console.log('Received command: ', data.args);  // Print params
      return {msg: 'hi'};  // Result to be sent to the caller
    });
    ```

- In the IDC (Interactive Device Console), choose your target device.
- Click the Flash button and wait until the hello message appears in the device log.
- Switch to the Auth tab. It lists your authentication tokens. There is one
  token created for you by default, shown together with the example `curl`
  command line invocation.
- Open the terminal and copy/paste a `curl` command. Add arguments and the
  prepend device ID to the destination:

    ```sh
    curl -u xxx -d '{"foo":123}' https://DEVICE_ID.api.mongoose-iot.com/Command1
    ```

- Switch back to the IDE browser tab, notice the log message from the device:

<img src="media/tut_control.png" width="100%">
