// Note: if you want to create RESTful handlers, use RPC API instead.
// With RPC, you'll get a RESTful endpoint for free, plus many other extras
// like ability to call your API via other transports like Websocket, MQTT,
// serial, etc.

let HTTP = {
  // **`HTTP.get_system_server()`** - return an opaque pointer variable,
  // a handler of the built-in HTTP server.
  get_system_server: ffi('void *mgos_get_sys_http_server()'),

  // **`HTTP.bind(portStr)`** - start HTTP listener. Return an opaque pointer.
  // Avoid using this, use `HTTP.get_system_server()` instead.
  bind: ffi('void *mgos_bind_http(char *)'),

  // **`HTTP.add_endpoint(listener, uri, handler)`** - register URI
  // handler. Avoid using this, use `RPC.addHandler()` instead.
  // Handler function is Mongoose event handler, which receives an opaque
  // connection, event number, and event data pointer.
  // Events are `HTTP.EV_REQUEST`, `HTTP.EV_RESPONSE`.
  // Return value: 1 in case of success, 0 otherwise.
  // Example:
  // ```javascript
  // let server = HTTP.get_system_server();
  // HTTP.add_endpoint(server, '/my/api', function(conn, ev, ev_data) {
  //   if (ev === HTTP.EV_REQUEST) {
  //     Net.send(conn, 'HTTP/1.0 200 OK\n\n  hello! \n');
  //     Net.disconnect(conn);
  //   }
  // }, null);
  // ```
  add_endpoint: ffi('int mgos_add_http_endpoint(void *, char *, void (*)(void *, int, void *, userdata), userdata)'),

  // **`HTTP.connect(addr, handler, userdata)`** - The same as `Net.connect`,
  // but with HTTP-specific handler attached, so that the callback can receive
  // additional events:
  // - `HTTP.EV_REQUEST`
  // - `HTTP.EV_RESPONSE`
  // - `HTTP.EV_CHUNK`
  // - `HTTP.EV_WS_HANDSHAKE`
  // - `HTTP.EV_WS_HANDSHAKE_DONE`
  // - `HTTP.EV_WS_FRAME`
  // - `HTTP.EV_WS_CONTROL_FRAME`
  // Return value: an opaque connection pointer which should be given as a
  // first argument to some `Net` functions.
  connect: ffi('void *mgos_connect_http(char *, void (*)(void *, int, void *, userdata), userdata)'),

  EV_REQUEST: 100,
  EV_RESPONSE: 101,
  EV_CHUNK: 102,

  EV_WS_HANDSHAKE: 111,
  EV_WS_HANDSHAKE_DONE: 112,
  EV_WS_FRAME: 113,
  EV_WS_CONTROL_FRAME: 114,

  // **`HTTP.param(event_data, param)`** - Get various params values of the
  // HTTP-specific events. When the callback given to `HTTP.connect()` is
  // called with the event `HTTP.EV_REQUEST` or `HTTP.EV_RESPONSE`,
  // `HTTP.param()` can be used to retrieve event details from the
  // `event_data`.
  //
  // Possible values of the `param` argument:
  // - `HTTP.METHOD`
  // - `HTTP.URI`
  // - `HTTP.PROTOCOL`
  // - `HTTP.BODY`
  // - `HTTP.MESSAGE`
  // - `HTTP.QUERY_STRING`
  //
  // Return value: a string with the param value.
  param: ffi('char *mgos_get_http_message_param(void *, int)'),
  METHOD: 0,
  URI: 1,
  PROTOCOL: 2,
  BODY: 3,
  MESSAGE: 4,
  QUERY_STRING: 5,
};
