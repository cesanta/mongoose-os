---
title: Configuration infrastructure
---

After boot, Mongoose IoT creates a WiFi Access Point (AP) called `FW_??????`,
where `??` are replaced by the hex numbers from device's MAC address.
On that AP, Mongoose IoT runs a web server on address `192.168.4.1` with
a configuration interface:

![](media/cfg.png)

- Join `FW_??????` AP and point your browser at http://192.168.4.1
- Change configuration parameters as needed, press Save.
- The module will reboot with new configuration.

It is possible to add custom parameters to the configuration UI - see section
below. Configuration parameters are accessible by JavaScript code through
`Sys.conf` variable.
