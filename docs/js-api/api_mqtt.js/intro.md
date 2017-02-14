---
title: "MQTT"
items:
---

 MQTT API. Source C API is defined at:
 [mgos_mqtt.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_mqtt.h)
 This API provides publish and subscribe functions. The MQTT server should
 be configured via the `mqtt` configuration section, or dynamically, like
 `mos config-set mqtt.server=test.mosquitto.org:1883`.



 **`MQTT.sub(topic, handler)`** - subscribe to a topic, and call given
 handler function when message arrives.

 Return value: none.

 Example:
 ```javascript
 MQTT.sub('my/topic', function(conn, message) {
   print('Got message:', message);
 });
 ```



 **`MQTT.pub(topic, message)`** - pubish message to a topic. Return value:
 0 on failure (e.g. no connection to server), 1 on success. Example:
 ```javascript
 let res = MQTT.pub('my/topic', JSON.stringify({ a: 1, b: 2 }));
 print('Published:', res ? 'yes' : 'no');
 ```

