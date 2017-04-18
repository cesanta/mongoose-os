// Simple level-based logging facility

let Log = {
  // **`Log.print(level, msg)`** - Print message to stderr if provided
  // level is >= `Cfg.get('debug.level')`
  print: ffi('void mgos_log(int, char *)'),

  // **`Log.error(msg)`** - Shortcut for `Log.print(Log.ERROR, msg)`
  error: function(msg) {
    this.print(this.ERROR, msg);
  },

  // **`Log.warn(msg)`** - Shortcut for `Log.print(Log.WARN, msg)`
  warn: function(msg) {
    this.print(this.WARN, msg);
  },

  // **`Log.info(msg)`** - Shortcut for `Log.print(Log.INFO, msg)`
  info: function(msg) {
    this.print(this.INFO, msg);
  },

  // **`Log.debug(msg)`** - Shortcut for `Log.print(Log.DEBUG, msg)`
  debug: function(msg) {
    this.print(this.DEBUG, msg);
  },

  // **`Log.verboseDebug(msg)`** - Shortcut for `Log.print(Log.VERBOSE_DEBUG, msg)`
  verboseDebug: function(msg) {
    this.print(this.VERBOSE_DEBUG, msg);
  },

  ERROR: 0,
  WARN: 1,
  INFO: 2,
  DEBUG: 3,
  VERBOSE_DEBUG: 4,
};
