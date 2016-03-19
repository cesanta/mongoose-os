---
title: Watchdog timer (esp8266)
---

ESP8266 includes a watchdog timer, a mechanism designed to deal with software
lockups. This timer is periodically reset by the OS, or can be explicitly "fed"
by user code. Failure to do so will cause the device to be reset.  The current
default watchdog timer period is about 1 minute.

- `Sys.wdtFeed()`: delay restart of the device for 1 minute. This function has
  to be called inside long loops, or other long operations to prevent device
  reset.

