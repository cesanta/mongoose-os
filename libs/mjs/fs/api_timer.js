load('api_math.js');

let Timer = {
  _f: ffi('int mgos_strftime(char *, int, char *, int)'),

  // ## **`Timer.set(milliseconds, flags, handler, userdata)`**
  // Setup timer with `milliseconds` timeout and `handler` as a callback.
  // `flags` can be either 0 or `Timer.REPEAT`. In the latter case, the call
  // will be repeated indefinitely (but can be cancelled with `Timer.del()`),
  // otherwise it's a one-off.
  //
  // Return value: numeric timer ID.
  //
  // Example:
  // ```javascript
  // // Call every second
  // Timer.set(1000, Timer.REPEAT, function() {
  //   let value = GPIO.toggle(2);
  //   print(value ? 'Tick' : 'Tock');
  // }, null);
  // ```
  set: ffi('int mgos_set_timer(int,int,void(*)(userdata),userdata)'),

  REPEAT: 1,
  RUN_NOW: 2,

  // ## **`Timer.now()`**
  // Return current time as double value, UNIX epoch (seconds since 1970).
  now: ffi('double mg_time(void)'),

  // ## **`Timer.del(id)`**
  // Cancel previously installed timer.
  del: ffi('void mgos_clear_timer(int)'),

  // ## **`Timer.fmt(fmt, time)`**
  // Formats the time 'time' according to the strftime-like format
  // specification 'fmt'. The strftime reference can be found e.g.
  // [here](http://www.cplusplus.com/reference/ctime/strftime/).
  // Example:
  // ```javascript
  // let now = Timer.now();
  // let s = Timer.fmt("Now it's %I:%M%p.", now);
  // print(s); // Example output: "Now it's 12:01AM."
  // ```
  fmt: function(fmt, time) {
    if (!fmt) return 'invalid format';
    let res = 0, t = Math.round(time || Timer.now()), s = '     ';
    while (res === 0) {
      res = this._f(s, s.length, fmt, t);
      if (res === -1) return 'invalid time';
      if (res === 0) s += '     ';
    }
    return s.slice(0, res);
  },
};
