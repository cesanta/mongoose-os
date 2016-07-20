---
title: Watchdog timer (ESP8266)
---

ESP8266 includes a watchdog timer. This is a mechanism designed to deal with software
lockups. This timer is periodically reset by the OS or can be explicitly "fed"
by user code. Failure to do so will cause the device to be reset. The current
default watchdog timer period is 10 seconds.

- `Sys.wdtFeed()`: delays the restart of the device for 10 seconds. This function has
  to be called inside long loops or other long operations to prevent a device
  reset.
- `Sys.wdtSetTimeout(timeout)`: changes the watchdog timer period to `timeout` seconds.


