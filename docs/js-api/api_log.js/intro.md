---
title: "Log"
items:
---

Simple level-based logging facility



**`Log.print(level, msg)`** - Print message to stderr if provided
level is >= `Cfg.get('debug.level')`



Frame number: we're starting from the third frame, ignoring the first
two:
- this._off() or this._fn()
- Log.print()



bcode offset of interest, and the corresponding function:lineno



We'll go up by call trace until we find the frame not from the current
file



Found the first frame from other file, we're done.



**`Log.error(msg)`** - Shortcut for `Log.print(Log.ERROR, msg)`



**`Log.warn(msg)`** - Shortcut for `Log.print(Log.WARN, msg)`



**`Log.info(msg)`** - Shortcut for `Log.print(Log.INFO, msg)`



**`Log.debug(msg)`** - Shortcut for `Log.print(Log.DEBUG, msg)`



**`Log.verboseDebug(msg)`** - Shortcut for `Log.print(Log.VERBOSE_DEBUG, msg)`

