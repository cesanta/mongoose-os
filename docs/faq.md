---
title: F.A.Q.
---

#####Q: Does Mongoose IoT firmware support ESP modules with 512Kb flash?<br>
  A: No. In order to  run Mongoose IoT you need module with at least 1MB flash.
<br><br>
#####Q: I flashed ESP module with Mongoose IoT. It is successfully booted and joined WiFi. But after reset the device reboots over and over.<br>
  A: Usually this is a problem with a flash size configuration.
     Try to explicitly set flash size while flashing.
     You can do it either by command line (`--esp8266-flash-size=...`, run `MFT --help` for details)
     or you can use `Settings->Advanced settings` menu option. Check `esp8266-flash-size` in dialog appeared
     and put flash size in this field.
<br><br>
#####Q: My ESP device is rebooted by watch dog<br>
  A: If your device permamently reboots because of watchdog, you have several options:
     1. If the device is rebooted during long loop in JS code, it is better to use
     `Sys.wdtFeed()` call inside this loop.
     2. If the device is rebooted during some internal operation (like SSL handshake),
     you can change WDT timeout, by using `Sys.wdtSetTimeout(timeout)` (will be effective
     until reboot) or by changing `sys.wdt_timeout` (seconds) configuration parameter.
