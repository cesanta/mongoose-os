let Blynk = {
  _send: ffi('void blynk_send(void *, int, int, void *, int)'),

  // ## `Blynk.send(conn, type, msg)`
  // Send raw message to Blynk server.
  send: function(conn, type, msg) {
    this._send(conn, type, 0, msg, msg.length);
  },

  // ## `Blynk.virtualWrite(conn, pin, val)`
  // Write to the virtual pin.
  virtualWrite: function(conn, pin, val) {
    let msg = 'vr\x00' + JSON.stringify(pin) + '\x00' + JSON.stringify(val);
    this.send(conn, 20, msg);
  },

  // ## `Blynk.setHandler(handler)`
  // Example:
  // ```javascript
  // Blynk.setHandler(function(conn, cmd, pin, val) {
  //   print(cmd, pin, val);
  // }, null);
  // ```
  setHandler: ffi('void blynk_set_handler(void (*)(void *, char *, int, int, userdata), userdata)'),
};
