// AWS API, see AWS documentation at
// http://docs.aws.amazon.com/iot/latest/developerguide/iot-thing-shadows.html
//
// Usage example:
//
// ```javascript
// load('api_aws.js');
// load('api_gpio.js');
//
// let state = { on: false, counter: 0 };  // device state: shadow metadata
//
// // Upon startup, report current actual state, "reported"
// // When cloud sends us a command to update state ("desired"), do it
// AWS.Shadow.setStateHandler(function(data, event, reported, desired) {
//   if (event === AWS.Shadow.CONNECTED) {
//     AWS.Shadow.update(0, {state: state});  // Report current state
//   } else if (event === AWS.Shadow.UPDATE_DELTA) {
//     for (let key in state) {
//       if (desired.state[key] !== undefined) state[key] = desired[key];
//     }
//   }
//   print(JSON.stringify(reported), JSON.stringify(desired));
// }, null);
//
// // On a button press, update press counter via the shadow
// let buttonPin = 0;
// GPIO.set_button_handler(buttonPin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
//   AWS.Shadow.update(0, {desired: {on: state.on, counter: state.counter + 1}});
// }, null);
// ```

let AWS = {
  Shadow: {
    _seth: ffi(
        'void mgos_aws_shadow_set_state_handler_simple(int (*)(userdata, int, char *, char *), userdata)'),
    _upd: ffi('int mgos_aws_shadow_update_simple(double, char *)'),
    _scb: function(ud, ev, rep, des) {
      rep = rep !== "" ? JSON.parse(rep) : {};
      des = des !== "" ? JSON.parse(des) : {};
      return ud.cb(ud.ud, ev, rep, des);
    },

    // ## **`AWS.Shadow.setStateHandler(callback, userdata)`**
    // Set AWS shadow
    // state handler callback. When AWS shadow state changes, the callback is
    // called with the following arguments: `(userdata, event, reported,
    // desired)`, where `userdata` is the userdata given to `setStateHandler`,
    // `event` is one of the following: `AWS.Shadow.CONNECTED`,
    // `AWS.Shadow.GET_ACCEPTED`,
    // `AWS.Shadow.GET_REJECTED`, `AWS.Shadow.UPDATE_ACCEPTED`,
    // `AWS.Shadow.UPDATE_REJECTED`, `AWS.Shadow.UPDATE_DELTA`.
    // `reported` is previously reported state object (if any), and `desired`
    // is the desired state (if present).
    setStateHandler: function(cb, ud) {
      this._seth(this._scb, {
        cb: cb,
        ud: ud,
      });
    },

    // ## **`AWS.Shadow.update(version, state)`**
    // Update AWS shadow state.
    // State should be an object with "reported" and/or "desired" keys.
    //
    // Response will arrive via `UPDATE_ACCEPTED` or `UPDATE_REJECTED` events.
    // If you want the update to be aplied only if a particular version is
    // current, specify the version. Otherwise set it to 0 to apply to any
    // version. Example: increase `state.counter` on a button press:
    update: function(ver, state) {
      return this._upd(ver, JSON.stringify(state)) === 1;
    },

    CONNECTED: 0,
    GET_ACCEPTED: 1,
    GET_REJECTED: 2,
    UPDATE_ACCEPTED: 3,
    UPDATE_REJECTED: 4,
    UPDATE_DELTA: 5,
  },
};
