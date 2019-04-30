load('api_bt_gatt.js');

let GATTS = {
  EV_CONNECT: 0,
  EV_READ: 1,
  EV_WRITE: 2,
  EV_NOTIFY_MODE: 3,
  EV_IND_CONFIRM: 4,
  EV_DISCONNECT: 5,

  // ## **`GATTS.registerService(uuid, secLevel, chars, handler)`**
  // Register a GATTS service.
  // `uuid` specifies the service UUID (in string form, "1234" for 16 bit UUIDs,
  // "12345678-90ab-cdef-0123-456789abcdef" for 128-bit).
  // `sec_level` specifies the minimum required security level of the connection.
  // `chars` is an array of characteristic definitions.
  // `handler` will receive the events pertaining to the connection,
  // including reads and writes for characteristics that do not specify a handler.
  //
  // Handler function takes conenction object, event and an argument
  // and should return a status code.
  registerService: function(uuid, secLevel, chars, handler) {
    let charsC = null;
    for (let i = 0; i < chars.length; i++) {
      // Note: per-char handlers are currently not supported in JS.
      charsC = GATTS._addc(charsC, chars[i][0], chars[i][1]);
    }
    let res = GATTS._rs(uuid, secLevel, charsC, function(c, ev, ea, h) {
      let co = s2o(c, GATTS._cd);
      co._c = c;
      let eao = ea;
      if (ev === GATTS.EV_READ) {
        eao = s2o(ea, GATTS._rad);
        eao._ra = ea;
      } else if (ev === GATTS.EV_WRITE) {
        eao = s2o(ea, GATTS._wad);
      } else if (ev === GATTS.EV_NOTIFY_MODE) {
        eao = s2o(ea, GATTS._nmad);
      }
      return h(co, ev, eao);
    }, handler);
    GATTS._fch(charsC);
    return res;
  },

  sendRespData: function(c, ra, data) {
    GATTS._srd(c._c, ra._ra, data);
  },

  notify: function(c, mode, handle, data) {
    GATTS._ntfy(c._c, mode, handle, data);
  },

  _cd: ffi('void *mgos_bt_gatt_js_get_conn_def(void)')(),
  _rad: ffi('void *mgos_bt_gatts_js_get_read_arg_def(void)')(),
  _wad: ffi('void *mgos_bt_gatts_js_get_write_arg_def(void)')(),
  _nmad: ffi('void *mgos_bt_gatts_js_get_notify_mode_arg_def(void)')(),
  _addc: ffi('void *mgos_bt_gatts_js_add_char(void *, char *, int)'),
  _fch: ffi('void mgos_bt_gatts_js_free_chars(void *)'),
  _rs: ffi('bool mgos_bt_gatts_register_service(char *, int, void *, int (*)(void *, int, void *, userdata), userdata)'),
  _srd: ffi('void mgos_bt_gatts_send_resp_data_js(void *, void *, struct mg_str *)'),
  _ntfy: ffi('void mgos_bt_gatts_notify_js(void *, int, int, struct mg_str *)'),
};
