// Raw TCP/UDP API.

let Net = {
  // ## **`Net.bind(addressStr, handler)`**
  // Bind to an address. Return value:
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

  // ## **`Net.connect(addr, handler, userdata)`**
  // Connect to a remote host.
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
  // `Net.send()`, `Net._send()`, `Net.close()`.
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

  // ## **`Net.connect_ssl(addr, handler, userdata, cert, key, ca_cert)`**
  // The same as `Net.connect`, but establishes SSL connection
  // Additional parameters are:
  // - `cert` is a client certificate file name or "" if not required
  // - `key` is a client key file name or "" if not required
  // - `ca_cert` is a CA certificate or "" if peer verification is not required.
  // The certificate files must be in PEM format.
  connect_ssl: ffi('void *mgos_connect_ssl(char *, void (*)(void *, int, void *, userdata), userdata, char *, char *, char *)'),

  // ## **`Net.close(conn)`**
  // Send all pending data to the remote peer,
  // and disconnect when all data is sent.
  // Return value: none.
  close: ffi('void mgos_disconnect(void *)'),

  _send: ffi('void mg_send(void *, void *, int)'),

  // ## **`Net.send(conn, data)`**
  // Send data to the remote peer. `data` is an mJS string.
  // Return value: none.
  send: function(c, msg) { return Net._send(c, msg, msg.length); },

  EV_POLL: 0,
  EV_ACCEPT: 1,
  EV_CONNECT: 2,
  EV_RECV: 3,
  EV_SEND: 4,
  EV_CLOSE: 5,
  EV_TIMER: 6,

  // ## **`Net.setStatusEventHandler(handler, data)`**
  // Set network status handler. A handler is a function that receives
  // events: `Net.STATUS_DISCONNECTED`, `Net.STATUS_CONNECTED`,
  // `Net.STATUS_GOT_IP`.
  setStatusEventHandler: ffi('void mgos_wifi_add_on_change_cb(void (*)(int, userdata), userdata)'),
  STATUS_DISCONNECTED: 0,
  STATUS_CONNECTED: 1,
  STATUS_GOT_IP: 2,
};
