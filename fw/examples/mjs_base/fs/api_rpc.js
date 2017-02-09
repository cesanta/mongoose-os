let RPC = {
  _addHandler: ffi('void *mgos_rpc_add_handler(char *, void (*)(void *, char *, char *, userdata), userdata)'),
  _sendResponse: ffi('int mgos_rpc_send_response(void *, char *)'),
  _call: ffi('int mg_rpc_call(char *, char *, char *, void (*)(char *, int, char *, userdata), userdata)'),

  /*
   * Internal shim callback which parses JSON string args and passes to the
   * real callback
   */
  _addCB: function(ri, args, src, ud) {
    let resp = ud.cb(JSON.parse(args), src, ud.ud);
    RPC._sendResponse(ri, JSON.stringify(resp));
  },

  /*
   * Internal shim callback which parses JSON string args and passes to the
   * real callback
   */
  _callCB: function(res, err_code, err_msg, ud) {
    ud.cb(JSON.parse(res), err_code, err_msg, ud.ud);
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

  /*
   * Performs RPC call. If dst is empty, cloud is implied. Method is a string
   * like "MyMethod", cb is a callback function which takes the following
   * arguments: res, err_code, err_msg, userdata.
   *
   * res is an object with results,
   * err_code is 0 in case of success, error code otherwise,
   * err_msg is relevant if only err_code is non-zero,
   * userdata is a value given as a userdata to call().
   */
  call: function(dst, method, args, cb, userdata) {
    return RPC._call(dst, method, JSON.stringify(args), RPC._callCB, {
      cb: cb,
      ud: userdata,
    });
  },
};

