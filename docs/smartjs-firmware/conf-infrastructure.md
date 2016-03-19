---
title: Configuration infrastructure
---

After boot, Smart.js creates a WiFi Access Point (AP) called `SMARTJS_??????`,
where `??` are replaced by the hex numbers from device's MAC address.
On that AP, Smart.js runs a web server on address `192.168.4.1` with
a configuration interface:

![](../../static/img/smartjs/cfg.png)

- Join `SMARTJS_??????` AP and point your browser at http://192.168.4.1
- Change configuration parameters as needed, press Save.
- The module will reboot with new configuration.

It is possible to add custom parameters to the configuration UI - see section
below. Configuration parameters are accessible by JavaScript code through
`Sys.conf` variable.

