let RPC = {
  _sdup: ffi('void *strdup(char *)'),
  _ah: ffi('void *mgos_rpc_add_handler(void *, void (*)(void *, char *, char *, userdata), userdata)'),
  _resp: ffi('bool mgos_rpc_send_response(void *, char *)'),
  _call: ffi('bool mgos_rpc_call(char *, char *, char *, void (*)(char *, int, char *, userdata), userdata)'),
  _err: ffi('bool mg_rpc_send_errorf(void *, int, char *, char *)'),

  _ahcb: function(ri, args, src, ud) {
    // NOTE(lsm): not using `this` here deliberately. Calleth from C.
    let resp = ud.cb(JSON.parse(args || 'null'), src, ud.ud);
    if (typeof(resp) === 'undefined') resp = null;
    if (typeof(resp) === 'object' && typeof(resp.error) === 'number') {
      RPC._err(ri, resp.error, '%s', resp.message || '');
    } else {
      RPC._resp(ri, JSON.stringify(resp));
    }
    // NOTE: we don't call ffi_cb_free here because this handler might be used
    // more than once
  },

  _ccb: function(res, code, msg, ud) {
    ud.cb(res ? JSON.parse(res) : null, code, msg, ud.ud);
    ffi_cb_free(RPC._ccb, ud);
  },

  LOCAL: "RPC.LOCAL",

  // ## **`RPC.addHandler(name, handler)`**
  // Add RPC handler. `name` is a string like `'MyMethod'`, `handler`
  // is a callback function which takes `args` arguments object.
  // If a handler returns an object with a numeric `error` attribute and
  // optional `message` string attribute, the caller will get a failure.
  //
  // Return value: none.
  //
  // Example:
  // ```javascript
  // RPC.addHandler('Sum', function(args) {
  //   if (typeof(args) === 'object' && typeof(args.a) === 'number' &&
  //       typeof(args.b) === 'number') {
  //     return args.a + args.b;
  //   } else {
  //     return {error: -1, message: 'Bad request. Expected: {"a":N1,"b":N2}'};
  //   }
  // });
  // ```
  addHandler: function(name, cb, ud) {
    let data = {cb: cb, ud: ud};
    // TODO(lsm): get rid of this strdup() leak. One-off, but still ugly.
    this._ah(this._sdup(name), this._ahcb, data);
  },

  // ## **`RPC.call(dst, method, args, callback)`**
  // Call remote or local RPC service.
  // Return value: true in case of success, false otherwise.
  //
  // If `dst` is empty, connected server is implied. `method` is a string
  // like "MyMethod", `callback` is a callback function which takes the following
  // arguments: res (results object), err_code (0 means success, or error code
  // otherwise), err_msg (error messasge for non-0 error code), userdata. Example:
  //
  // ```javascript
  // RPC.call(RPC.LOCAL, 'Config.Save', {reboot: true}, function (resp, ud) {
  //   print('Response:', JSON.stringify(resp));
  // }, null);
  // ```
  call: function(dst, name, args, cb, ud) {
    let data = {cb: cb, ud: ud};
    return this._call(dst, name, JSON.stringify(args), this._ccb, data);
  },
};

