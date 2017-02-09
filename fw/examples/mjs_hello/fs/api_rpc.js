let RPC = {
  _addHandler: ffi('void *mgos_rpc_add_handler(char *, void (*)(void *, char *, char *, userdata), userdata)'),
  _sendResponse: ffi('int mgos_rpc_send_response(void *, char *)'),

  /*
   * Internal shim callback which parses JSON string args and passes to the
   * real callback
   */
  _addCB: function(ri, args, src, ud) {
    let resp = ud.cb(JSON.parse(args), src, ud.ud);
    RPC._sendResponse(ri, JSON.stringify(resp));
  },

  /*
   * Adds RPC handler. Method is a string like "MyMethod", cb is a callback
   * function which takes the following arguments: args, src, userdata.
   *
   * args is an object with arguments,
   * src is a string with the source of request,
   * userdata is a value given as a userdata to addHandler.
   */
  addHandler: function(method, cb, userdata) {
    RPC._addHandler(method, RPC._addCB, {
      cb: cb,
      ud: userdata,
    });
  },
};
