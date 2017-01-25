let HTTP = {
  rbind: ffi('void *mgos_bind_http(char *, void (*)(void *, int, void *, userdata), userdata)'),
  lcb: function(conn, ev, ev_data, data) {
    if (ev === 100) data.cb(conn, ev_data);
  },
  listen: function(addr, cb) {
    let data = { cb: cb };
    HTTP.rbind(addr, HTTP.lcb, data);
  },

  rconnect: ffi('void *mgos_connect_http(char *, void (*)(void *, int, void *, userdata), userdata)'),

  // HTTP Events
  EV_REQUEST: 100,
  EV_REPLY: 101,
  EV_CHUNK: 102,

  // Websocket events
  EV_WS_HANDSHAKE: 111,
  EV_WS_HANDSHAKE_DONE: 112,
  EV_WS_FRAME: 113,
  EV_WS_CONTROL_FRAME: 114,

  // Get HTTP request parameters
  param: ffi('char *mgos_get_http_message_param(void *, int)'),
  METHOD: 0,
  URI: 1,
  PROTOCOL: 2,
  BODY: 3,
  MESSAGE: 4,
  QUERY_STRING: 5,
};
