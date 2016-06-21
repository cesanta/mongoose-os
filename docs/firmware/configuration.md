---
title: Configuration
---

In JavaScript, system boot process loads configuration into `Sys.conf`
object. To make changes to configuration, update the respective value
(for example, `Sys.conf.wifi.ap.enable = false;`) and call `Sys.conf.save()`
function. That call will update `conf.json` file and reboot the firmare.

In C, configuration object is accessible via the `get_cfg()` function. See an
[example on how make and use custom configuration parameter in C](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/src/app_main.c#L19).

Apart from programmatic access, configuration can be accessed via the
Web UI. If your device has joined your local WiFi network, point your browser
at http://YOUR-DEVICE-IP. If your device has started it's own WiFi network,
join it and point your browser at http://192.168.4.1. Then, make any
required change and press Save button.

To list all configuration parameters and their meaning, please see
[conf_sys_schema.json](https://github.com/cesanta/dev/blob/master/fw/src/fs/conf_sys_schema.json) file. This file is used by the Web UI to render configuration parameters
and their description.
