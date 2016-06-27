---
title: Send data to an MQTT server
---

This example shows how to send periodic measurements to some external
MQTT server. This example uses http://httpbin.org, which is a useful public
RESTful service that allows to test or debug RESTful interfaces.


- Login to [Mongoose Cloud](https://mongoose-iot.com)
- Create a new project, call it `mqtt`
- Swith to the IDE tab
- Copy/paste the following code into the `app.js`

    ```javascript
    console.log('Hello from mqtt example');

    var c = MQTT.connect('mqtt://test.mosquitto.org:1883');

    c.on('error', function() {
      console.log('MQTT error', arguments);
    });

    c.on('message', function(topic, message) {
      console.log("MQTT message: ", topic, ' - [', topic, ']');
    });

    c.on('connect', function() {
      console.log('connected');
      c.subscribe('/foo');
      c.publish('/foo', 'Hello from Mongoose Firmware');
    });
    ```

- In the IDC (Interactive Device Console), choose your target device
- Click Flash button, and wait until messages start to appear
