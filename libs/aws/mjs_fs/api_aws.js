let AWS = {
  // ## **`AWS.isConnected()`**
  // Return value: true if AWS connection is up, false otherwise.
  isConnected: ffi('bool mgos_aws_is_connected()'),

  Shadow: {
    _seth: ffi('void mgos_aws_shadow_set_state_handler_simple(int (*)(userdata, int, char *, char *, char *, char *), userdata)'),
    _upd: ffi('int mgos_aws_shadow_update_simple(double, char *)'),
    _scb: function(ud, ev, rep, des, rm, dm) {
      rep = rep !== "" ? JSON.parse(rep) : {};
      des = des !== "" ? JSON.parse(des) : {};
      ud.cb(ud.ud, ev, rep, des, rm, dm);
    },

    // ## **`AWS.Shadow.setStateHandler(callback, userdata)`**
    //
    // Set AWS shadow state handler callback.
    //
    // When AWS shadow state changes, the callback is
    // called with the following arguments: `(userdata, event, reported,
    // desired, reported_metadata, desired_metadata)`,
    // where `userdata` is the userdata given to `setStateHandler`,
    // `event` is one of the following: `AWS.Shadow.CONNECTED`,
    // `AWS.Shadow.GET_ACCEPTED`,
    // `AWS.Shadow.GET_REJECTED`, `AWS.Shadow.UPDATE_ACCEPTED`,
    // `AWS.Shadow.UPDATE_REJECTED`, `AWS.Shadow.UPDATE_DELTA`.
    // `reported` is previously reported state object (if any), and `desired`
    // is the desired state (if present).
    //
    // Example:
    // ```javascript
    // let state = { on: false, counter: 0 };  // device state: shadow metadata
    //
    // // Upon startup, report current actual state, "reported"
    // // When cloud sends us a command to update state ("desired"), do it
    // AWS.Shadow.setStateHandler(function(data, event, reported, desired, reported_metadata, desired_metadata) {
    //   if (event === AWS.Shadow.CONNECTED) {
    //     AWS.Shadow.update(0, state);  // Report device state
    //   } else if (event === AWS.Shadow.UPDATE_DELTA) {
    //     for (let key in state) {
    //       if (desired[key] !== undefined) state[key] = desired[key];
    //     }
    //     AWS.Shadow.update(0, state);  // Report device state
    //   }
    //   print(JSON.stringify(reported), JSON.stringify(desired));
    // }, null);
    // ```
    setStateHandler: function(cb, ud) {
      this._seth(this._scb, {
        cb: cb,
        ud: ud,
      });
    },

    // ## **`AWS.Shadow.get()`**
    //
    // Request shadow state. The event handler (see
    // `AWS.Shadow.setStateHandler()`) will receive a `GET_ACCEPTED` event.
    // Returns true in case of success, false otherwise.
    get: ffi('bool mgos_aws_shadow_get()'),

    // ## **`AWS.Shadow.getVersion();`**
    // Return last shadow state version (a number).
    getVersion: ffi('double mgos_aws_shadow_get_last_state_version(void)'),


    // ## **`AWS.Shadow.update(version, state);`**
    //
    // Update AWS shadow state.
    //
    // State should be an object with "reported" and/or "desired" keys.
    //
    // Response will arrive via `UPDATE_ACCEPTED` or `UPDATE_REJECTED` events.
    // If you want the update to be aplied only if a particular version is
    // current, specify the version. Otherwise set it to 0 to apply to any
    // version.
    //
    // Example:
    // ```javascript
    // // On a button press, update press counter via the shadow
    // let buttonPin = 0;
    // GPIO.set_button_handler(buttonPin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
    //   AWS.Shadow.update(0, {desired: {on: state.on, counter: state.counter + 1}});
    // }, null);
    // ```
    update: function(ver, state) {
      return this._upd(ver, JSON.stringify(state)) === 1;
    },

    eventName: ffi('char *mgos_aws_shadow_event_name(int)'),

    CONNECTED: 0,
    GET_ACCEPTED: 1,
    GET_REJECTED: 2,
    UPDATE_ACCEPTED: 3,
    UPDATE_REJECTED: 4,
    UPDATE_DELTA: 5,
  },
};
