let GAP = {
  EV_GRP: Event.baseNumber('GAP'),

  scan: ffi('bool mgos_bt_gap_scan_js(int, bool)'),
  getScanResultArg: function(evdata) { return s2o(evdata, GAP._srdd) },

  // ## **`GAP.parseName(advData)`**
  // Parse name from adv data. Tries to get long, falls back to short.
  parseName: ffi('char *mgos_bt_gap_parse_name_js(struct mg_str *)'),

  _srdd: ffi('void *mgos_bt_gap_get_srdd(void)')(),
};

GAP.EV_SCAN_RESULT = GAP.EV_GRP + 0;
GAP.EV_SCAN_STOP   = GAP.EV_GRP + 1;
