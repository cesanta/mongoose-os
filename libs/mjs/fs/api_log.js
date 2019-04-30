let Log = {
  // ## **`Log.print(level, msg)`**
  // Print message to stderr if provided
  // level is >= `Cfg.get('debug.level')`. Possible levels are:
  // - `Log.ERROR` (0)
  // - `Log.WARN` (1)
  // - `Log.INFO` (2)
  // - `Log.DEBUG` (3)
  // - `Log.VERBOSE_DEBUG` (4)
  print: function(level, msg) {
    let mjs = getMJS();
    // Frame number: we're starting from the third frame, ignoring the first
    // two:
    // - this._off() or this._fn()
    // - Log.print()
    let cfn = 2;

    // bcode offset of interest, and the corresponding function:lineno
    let offs, fn, ln;

    // We'll go up by call trace until we find the frame not from the current
    // file
    while (true) {
      offs = this._off(mjs, cfn) - 1;
      fn = this._fn(mjs, offs);
      if (fn !== "api_log.js") {
        // Found the first frame from other file, we're done.
        break;
      }
      cfn++;
    }
    ln = this._ln(mjs, offs);
    this._pr(fn, ln, level, msg);
  },

  // ## **`Log.error(msg)`**
  // Shortcut for `Log.print(Log.ERROR, msg)`
  error: function(msg) {
    this.print(this.ERROR, msg);
  },

  // ## **`Log.warn(msg)`**
  // Shortcut for `Log.print(Log.WARN, msg)`
  warn: function(msg) {
    this.print(this.WARN, msg);
  },

  // ## **`Log.info(msg)`**
  // Shortcut for `Log.print(Log.INFO, msg)`
  info: function(msg) {
    this.print(this.INFO, msg);
  },

  // ## **`Log.debug(msg)`**
  // Shortcut for `Log.print(Log.DEBUG, msg)`
  debug: function(msg) {
    this.print(this.DEBUG, msg);
  },

  // ## **`Log.verboseDebug(msg)`**
  // Shortcut for `Log.print(Log.VERBOSE_DEBUG, msg)`
  verboseDebug: function(msg) {
    this.print(this.VERBOSE_DEBUG, msg);
  },

  ERROR: 0,
  WARN: 1,
  INFO: 2,
  DEBUG: 3,
  VERBOSE_DEBUG: 4,

  _pr: ffi('void mgos_log(char *, int, int, char *)'),
  _fn: ffi('char *mjs_get_bcode_filename_by_offset(void *, int)'),
  _ln: ffi('int mjs_get_lineno_by_offset(void *, int)'),
  _off: ffi('int mjs_get_offset_by_call_frame_num(void *, int)'),
};
