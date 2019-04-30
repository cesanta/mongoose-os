let Dash = {
  // ## **`Dash.isConnected()`**
  // Return value: true if mDash connection is up, false otherwise.
  isConnected: ffi('bool mgos_dash_is_connected()'),

  // ## **`Dash.notify(name, data)`**
  // Send notification event to mDash. `name` is an event name,
  // `data` is either a string or an object. A string is sent as-is,
  // and object gets `JSON.stringify()`-ed then sent.
  //
  // Return value: none.
  //
  // Example:
  // ```javascript
  // Dash.notify('Data', {temperature: 12.34});
  // ```
  notify: function(name, data) {
    if (typeof (data) != 'string') data = JSON.stringify(data);
    let data = {cb: cb, ud: ud};
    this._no(name, data);
  },

  _no: ffi('void mgos_dash_notify(char *, char *)'),
};
