---
title: F.A.Q.
---

Q: Does Mongoose Firmware support ESP modules with 512Kb flash?<br>
A: No. In order to  run Mongoose Firmware you need a module with at least 1MB flash.


Q: I flashed an ESP module with Mongoose Firmware.
  It successfully booted and joined WiFi.
  But after reset the device reboots over and over.<br>
A: Usually this is a problem with the flash size configuration.
   Try to explicitly set flash size while flashing.
   You can do it either by using command line (`--esp8266-flash-size=...`,
   run `MFT --help` for details)
   or you can use the `Settings->Advanced settings` menu option. Check `esp8266-flash-size` in dialog that appears
   and put flash size in this field.


Q: My ESP device is rebooted by watch dog.<br>
A: If your device permamently reboots because of watchdog, you have several options:
   1. If the device is rebooted during long loop in JS code, it is better to use a
   `Sys.wdtFeed()` call inside this loop.
   2. If the device is rebooted during some internal operation (like an SSL handshake),
   you can change the WDT timeout by using `Sys.wdtSetTimeout(timeout)` (will be effective
   until reboot) or by changing the `sys.wdt_timeout` (seconds) configuration parameter.
