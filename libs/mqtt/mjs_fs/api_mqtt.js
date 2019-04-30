let MQTT = {
  _sub: ffi('void mgos_mqtt_sub(char *, void (*)(void *, void *, int, void *, int, userdata), userdata)'),
  _subf: function(conn, topic, len1, msg, len2, ud) {
    return ud.cb(conn, mkstr(topic, len1), mkstr(msg, len2), ud.ud);
  },

  // ## **`MQTT.isConnected()`**
  // Return value: true if MQTT connection is up, false otherwise.
  isConnected: ffi('bool mgos_mqtt_global_is_connected()'),

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

  _pub: ffi('int mgos_mqtt_pub(char *, void *, int, int, bool)'),

  // ## **`MQTT.pub(topic, message, qos, retain)`**
  // Publish message to a topic. If `qos` is not specified, it defaults to 0.
  // If `retain` is not specified, it defaults to `false`.
  // Return value: 0 on failure (e.g. no connection to server), 1 on success.
  //
  // Example - send MQTT message on button press, with QoS 1, no retain:
  // ```javascript
  // load('api_mqtt.js');
  // load('api_gpio.js');
  // let pin = 0, topic = 'my/topic';
  // GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
  //   let res = MQTT.pub('my/topic', JSON.stringify({ a: 1, b: 2 }), 1);
  //   print('Published:', res ? 'yes' : 'no');
  // }, null);
  // ```
  pub: function(t, m, qos, retain) {
    qos = qos || 0;
    return this._pub(t, m, m.length, qos, retain || false);
  },

  // ## **`MQTT.setEventHandler(handler, userdata)`**
  // Set MQTT connection event handler. Event handler is
  // `ev_handler(conn, ev, edata)`, where `conn` is an opaque connection handle,
  // `ev` is an event number, `edata` is an event-specific data.
  // `ev` values could be low-level network events, like `Net.EV_CLOSE`
  // or `Net.EV_POLL`, or MQTT specific events, like `MQTT.EV_CONNACK`.
  //
  // Example:
  // ```javascript
  // MQTT.setEventHandler(function(conn, ev, edata) {
  //   if (ev !== 0) print('MQTT event handler: got', ev);
  // }, null);
  // ```
  setEventHandler: ffi('void mgos_mqtt_add_global_handler(void (*)(void *, int, void *, userdata), userdata)'),

  // Event codes.
  EV_CONNACK: 202,   // Connection to broker has been established.
  EV_PUBLISH: 203,   // A message has been published to one of the topics we are subscribed to.
  EV_PUBACK: 204,    // Ack for publishing of a message with QoS > 0.
  EV_SUBACK: 209,    // Ack for a subscribe request.
  EV_UNSUBACK: 211,  // Ack for an unsubscribe request.
  EV_CLOSE: 5,       // Connection to broker was closed.
};
