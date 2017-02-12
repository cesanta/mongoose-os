// Raw TCP/UDP API.

let Net = {
  // **`Net.bind(addressStr, handler)`** - bind to an address. Return value:
  // an opaque pointer. A handler function is a Mongoose event handler,
  // that receives connection, event, and event data. Events are:
  // `Net.EV_POLL`, `Net.EV_ACCEPT`, `Net.EV_CONNECT`, `Net.EV_RECV`,
  // `Net.EV_SEND`, `Net.EV_CLOSE`, `Net.EV_TIMER`. Example:
  // ```javascript
  // Net.bind(':1234', function(conn, ev, ev_data) {
  //   print(ev);
  // }, null);
  // ```
  bind: ffi('void *mgos_bind(char *, void (*)(void *, int, void *, userdata), userdata)'),

  connect: ffi('void *mgos_connect(char *, void (*)(void *, int, void *, userdata), userdata)'),

  // **`Net.disconnect(conn)`** - send all pending data to the remote peer,
  // and disconnect when all data is sent.
  disconnect: ffi('void mgos_disconnect(void *)'),
  rsend: ffi('int mg_send(void *, char *, int)'),

  // **`Net.send(conn, data)`** - send data to the remote peer.
  send: function(c, msg) { return Net.rsend(c, msg, msg.length); },
  EV_POLL: 0,
  EV_ACCEPT: 1,
  EV_CONNECT: 2,
  EV_RECV: 3,
  EV_SEND: 4,
  EV_CLOSE: 5,
  EV_TIMER: 6,
};
