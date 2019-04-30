load('api_events.js');

let Azure = {
  EVENT_GRP: Event.baseNumber('AZR'),
  _c2dd: ffi('void *mgos_azure_get_c2dd(void)')(),

  // ## **`Azure.isConnected()`**
  // Return value: true if Azure connection is up, false otherwise.
  isConnected: ffi('bool mgos_azure_is_connected()'),

  getC2DArg: function(evdata) { return s2o(evdata, Azure._c2dd) },

  // ## **`Azure.sendD2CMsg(props, body)`**
  // Send a Device to Cloud message. `props`, if specified, must be URL-encoded.
  sendD2CMsg: ffi('bool mgos_azure_send_d2c_msgp(struct mg_str *, struct mg_str *)'),
};

Azure.EV_CONNECT = Azure.EVENT_GRP + 0;
Azure.EV_C2D     = Azure.EVENT_GRP + 1;
Azure.EV_DM      = Azure.EVENT_GRP + 2;
Azure.EV_CLOSE   = Azure.EVENT_GRP + 3;
