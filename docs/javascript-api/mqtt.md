---
title: MQTT
---

Mongoose IoT Platform provides a simple MQTT client. The API is modeled after the popular JS
MQTT client https://www.npmjs.com/package/mqtt.

Usage example:

```javascript
var client = MQTT.connect('mqtt://test.mosquitto.org');

client.on('connect', function() {
   client.subscribe('/foo');
   client.publish('/bar', 'Hello mqtt');
});

client.on('message', function(topic, message) {
   console.log("got", message);
   client.publish("/baz", message);
});
```

