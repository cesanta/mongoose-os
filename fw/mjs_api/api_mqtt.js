// MQTT API. Source C API is defined at:
// [mgos_mqtt.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_mqtt.h)
//
// This API provides publish and subscribe functions. The MQTT server should
// be configured via the `mqtt` configuration section, or dynamically, like
// `mos config-set mqtt.server=broker.hivemq.com:1883`.

let MQTT = {
  _sub: ffi('void mgos_mqtt_sub(char *, void (*)(void *, void *, int, void *, int, userdata), userdata)'),
  _subf: function(conn, topic, len1, msg, len2, ud) {
    return ud.cb(conn, fstr(topic, len1), fstr(msg, len2), ud.ud);
  },

  // ## **`MQTT.sub(topic, handler)`**
  // Subscribe to a topic, and call given handler function when message arrives.
  // A handler receives 4 parameters: MQTT connection, topic name,
  // message, and userdata.
  // Return value: none.
  //
  // Example:
  // ```javascript
  // load('api_mqtt.js');
  // MQTT.sub('my/topic/#', function(conn, topic, msg) {
  //   print('Topic:', topic, 'message:', msg);
  // }, null);
  // ```
  sub: function(topic, cb, ud) {
    return this._sub(topic, this._subf, { cb: cb, ud: ud });
  },

  _pub: ffi('int mgos_mqtt_pub(char *, void *, int, int)'),

  // ## **`MQTT.pub(topic, message, qos)`**
  // Publish message to a topic. QoS defaults to 0.
  // Return value: 0 on failure (e.g. no connection to server), 1 on success.
  //
  // Example - send MQTT message on button press:
  // ```javascript
  // load('api_mqtt.js');
  // load('api_gpio.js');
  // let pin = 0, topic = 'my/topic';
  // GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
  //   let res = MQTT.pub('my/topic', JSON.stringify({ a: 1, b: 2 }), 1);
  //   print('Published:', res ? 'yes' : 'no');
  // }, null);
  // ```
  pub: function(t, m, qos) {
    qos = qos || 0;
    return this._pub(t, m, m.length, qos);
  },
};
