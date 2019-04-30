load('api_bt_gatt.js');

let GATTC = {
  EV_GRP: Event.baseNumber('GAC'),

  connect: ffi('bool mgos_bt_gattc_connect_js(char *)'),
  getConnectArg: function(evdata) { return s2o(evdata, GATTC._cd) },

  discover: ffi('bool mgos_bt_gattc_discover(int)'),
  getDiscoveryResultArg: function(evdata) { return s2o(evdata, GATTC._drad) },

  read: ffi('bool mgos_bt_gattc_read(int, int)'),
  getReadResult: function(evdata) { return s2o(evdata, GATTC._rrd) },

  // NB: does not work at present, TODO
  subscribe: ffi('bool mgos_bt_gattc_subscribe(int, int)'),
  getNotifyArg: function(evdata) { return s2o(evdata, GATTC._nad) },

  // NB: does not work at present, TODO
  write: ffi('bool mgos_bt_gattc_write_js(int, int, struct mg_str *)'),

  disconnect: ffi('bool mgos_bt_gattc_disconnect(int)'),

  _cd: ffi('void *mgos_bt_gatt_js_get_conn_def(void)')(),
  _rrd: ffi('void *mgos_bt_gattc_js_get_read_result_def(void)')(),
  _nad: ffi('void *mgos_bt_gattc_js_get_notify_arg_def(void)')(),
  _drad: ffi('void *mgos_bt_gattc_js_get_discovery_result_arg_def(void)')(),
};

GATTC.EV_CONNECT          = GATTC.EV_GRP + 0;
GATTC.EV_DISCONNECT       = GATTC.EV_GRP + 1;
GATTC.EV_DISCOVERY_RESULT = GATTC.EV_GRP + 2;
GATTC.EV_READ_RESULT      = GATTC.EV_GRP + 3;
GATTC.EV_NOTIFY           = GATTC.EV_GRP + 4;
