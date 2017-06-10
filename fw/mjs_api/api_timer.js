load('api_math.js');

let Timer = {
  _f: ffi('int mgos_strftime(char *, int, char *, int)'),

  // ## **`Timer.set(milliseconds, repeat, handler)`**
  // Setup timer with `milliseconds` timeout and `handler` as a callback.
  // If `repeat` is set to true, the call will be repeated indefinitely,
  // otherwise it's a one-off.
  //
  // Return value: numeric timer ID.
  //
  // Example:
  // ```javascript
  // // Call every second
  // Timer.set(1000, true, function() {
  //   let value = GPIO.toggle(2);
  //   print(value ? 'Tick' : 'Tock');
  // }, null);
  // ```
  set: ffi('int mgos_set_timer(int,bool,void(*)(userdata),userdata)'),

  // ## **`Timer.now()`**
  // Return current time as double value, UNIX epoch (seconds since 1970).
  now: ffi('double mg_time(void)'),

  // ## **`Timer.del(id)`**
  // Cancel previously installed timer.
  del: ffi('void mgos_clear_timer(int)'),

  // ## **`Timer.fmt(fmt, time)`**
  // Formats the time 'time' according to the strftime-like format
  // specification 'fmt'
  fmt: function(fmt, time) {
    if (!fmt) return 'invalid format';
    let res = 0, t = Math.round(time || Timer.now()),  s = '     ';
    while (res === 0) {
      res = this._f(s, s.length, fmt, t);
      if (res === -1) return 'invalid time';
      if (res === 0) s += '     ';
    }
    return s;
  },
};
