let HTTP = {
  get_system_server: ffi('void *mgos_get_sys_http_server()'),
  bind: ffi('void *mgos_bind_http(char *)'),
  add_endpoint: ffi('int mgos_add_http_endpoint(void *, char *, void (*)(void *, int, void *, userdata), userdata)'),
  connect: ffi('void *mgos_connect_http(char *, void (*)(void *, int, void *, userdata), userdata)'),

  // HTTP Events
  EV_REQUEST: 100,
  EV_RESPONSE: 101,
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
