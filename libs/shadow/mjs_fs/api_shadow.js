load('api_events.js');

let Shadow = {
  BASE: Event.baseNumber('SDW'),
  _en: ffi('char *mgos_shadow_event_name(int)'),
  _sp: ffi('void *mgos_get_mgstr_ptr(void *)'),
  _sl: ffi('int mgos_get_mgstr_len(void *)'),
  _upd: ffi('bool mgos_shadow_update(double, char *)'),

  // ## **`Shadow.addHandler(callback)`**
  // Set up shadow event handler. Callback receives `event, obj` parameters.
  // Possibble values for `event` are:
  // `CONNECTED`,  `UPDATE_ACCEPTED`, `UPDATE_REJECTED`,`UPDATE_DELTA`.
  // `obj` is an shadow object, valid for `UPDATE_DELTA` and `UPDATE_ACCEPTED`
  // events.
  // See https://github.com/mongoose-os-apps/example-shadow-js for the
  // idiomatic usage.
  addHandler: function(cb) {
    Event.addGroupHandler(this.BASE, function(_e, _ed, _ud) {
      let n = _e - _ud.self.BASE, param = {};
      let name = _ud.self._en(_e) || '???';
      if (n === 3 || n === 5 || n === 7) {
        let s = mkstr(_ud.self._sp(_ed), _ud.self._sl(_ed));
        if (s) param = JSON.parse(s);
      }
      _ud.cb(name, param);
    }, {cb: cb, self: this});
  },

  // ## **`Shadow.get()`**
  // Ask cloud for the shadow. The reply will come as either `GET_ACCEPTED`
  // event or `GET_REJECTED` event.
  get: ffi('bool mgos_shadow_get(void)'),

  // ## **`Shadow.update()`**
  // Send shadow update. The idiomatic way of using shadow is: a) catch
  // `CONNECTED` event and report the current state, and b) catch `UPDATE_DELTA`
  // event, apply the delta, and report the state. Example:
  // ```javascript
  // Shadow.update(0, {temperature: 12.34});
  // ```
  update: function(version, obj) {
    this._upd(version, JSON.stringify(obj));
  },
};

let Twin = Shadow;
