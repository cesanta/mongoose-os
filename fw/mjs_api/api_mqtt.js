// MQTT API. Source C API is defined at:
// [mgos_mqtt.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_mqtt.h)
// This API provides publish and subscribe functions. The MQTT server should
// be configured via the `mqtt` configuration section, or dynamically, like
// `mos config-set mqtt.server=broker.hivemq.com:1883`.

let MQTT = {
  // **`MQTT.sub(topic, handler)`** - subscribe to a topic, and call given
  // handler function when message arrives.
  //
  // Return value: none.
  //
  // Example:
  // ```javascript
  // MQTT.sub('dfrank_topic', function(conn, topic_ptr, topic_len, msg_ptr, msg_len) {
  //   print(
  //     'Got message:',
  //     fstr(msg_ptr, msg_len),
  //     'from topic:',
  //     fstr(topic_ptr, topic_len)
  //   );
  // }, null);
  // ```
  sub: ffi('void mgos_mqtt_sub(char *, void (*)(void *, void *, int, void *, int, userdata), userdata)'),

  _pub: ffi('int mgos_mqtt_pub(char *, char *, int)'),

  // **`MQTT.pub(topic, message)`** - pubish message to a topic. Return value:
  // 0 on failure (e.g. no connection to server), 1 on success. Example:
  // ```javascript
  // let res = MQTT.pub('my/topic', JSON.stringify({ a: 1, b: 2 }));
  // print('Published:', res ? 'yes' : 'no');
  // ```
  pub: function(t, m) { return MQTT._pub(t, m, m.length); },
};
