// Raw TCP/UDP API.

let Net = {
  // **`Net.bind(addressStr, handler)`** - bind to an address. Return value:
  // an opaque connection pointer which should be given as a first argument to
  // some `Net` functions. A handler function is a Mongoose event
  // handler, that receives connection, event, and event data. Events are:
  // `Net.EV_POLL`, `Net.EV_ACCEPT`, `Net.EV_CONNECT`, `Net.EV_RECV`,
  // `Net.EV_SEND`, `Net.EV_CLOSE`, `Net.EV_TIMER`. Example:
  // ```javascript
  // Net.bind(':1234', function(conn, ev, ev_data) {
  //   print(ev);
  // }, null);
  // ```
  bind: ffi('void *mgos_bind(char *, void (*)(void *, int, void *, userdata), userdata)'),

  // **`Net.connect(addr, handler, userdata)`** - Connect to a remote host.
  // Return value: an opaque connection pointer which should be given as a first argument to
  // some `Net` functions.
  //
  // The addr format is `[PROTO://]HOST:PORT`. `PROTO` could be `tcp` or
  // `udp`. `HOST` could be an IP address or a host name. If `HOST` is a name,
  // it will be resolved asynchronously.
  //
  // Examples of valid addresses: `google.com:80`, `udp://1.2.3.4:53`,
  // `10.0.0.1:443`, `[::1]:80`.
  //
  // Handler is a callback function which takes the following arguments:
  // `(conn, event, event_data, userdata)`.
  // conn is an opaque pointer which should be used as a first argument to
  // `Net.send()`, `Net.rsend()`, `Net.disconnect()`.
  // event is one of the following:
  // - `Net.EV_POLL`
  // - `Net.EV_ACCEPT`
  // - `Net.EV_CONNECT`
  // - `Net.EV_RECV`
  // - `Net.EV_SEND`
  // - `Net.EV_CLOSE`
  // - `Net.EV_TIMER`
  // event_data is additional data; handling of it is currently not possible
  // from mJS.
  // userdata is the value given as a third argument to `Net.connect()`.
  connect: ffi('void *mgos_connect(char *, void (*)(void *, int, void *, userdata), userdata)'),

  // **`Net.disconnect(conn)`** - send all pending data to the remote peer,
  // and disconnect when all data is sent.
  // Return value: none.
  disconnect: ffi('void mgos_disconnect(void *)'),
  // **`Net.rsend(conn, data, length)`** - send byte array to the given
  // connection. Avoid using it, use `Net.send()` instead.
  // Return value: none.
  rsend: ffi('void mg_send(void *, char *, int)'),

  // **`Net.send(conn, data)`** - send data to the remote peer.
  // `data` is an mJS string.
  // Return value: none.
  send: function(c, msg) { return Net.rsend(c, msg, msg.length); },
  EV_POLL: 0,
  EV_ACCEPT: 1,
  EV_CONNECT: 2,
  EV_RECV: 3,
  EV_SEND: 4,
  EV_CLOSE: 5,
  EV_TIMER: 6,
};
