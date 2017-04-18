---
title: "Log"
items:
---

Simple level-based logging facility



**`Log.print(level, msg)`** - Print message to stderr if provided
level is >= `Cfg.get('debug.level')`



**`Log.error(msg)`** - Shortcut for `Log.print(Log.ERROR, msg)`



**`Log.warn(msg)`** - Shortcut for `Log.print(Log.WARN, msg)`



**`Log.info(msg)`** - Shortcut for `Log.print(Log.INFO, msg)`



**`Log.debug(msg)`** - Shortcut for `Log.print(Log.DEBUG, msg)`



**`Log.verboseDebug(msg)`** - Shortcut for `Log.print(Log.VERBOSE_DEBUG, msg)`

