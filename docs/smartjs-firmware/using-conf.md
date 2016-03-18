---
title: Using configuration
---

Configuration is accesible from JS through `Sys.conf` object. `Sys.conf`
contains all values from `conf_sys_defaults.json` and `conf.json` files in the
form of sub-objects and properties.  For example, `Sys.conf.wifi.sta.enable`
value turns on/off connection to Wifi. Value can be changed with usual JS
expression: `Sys.conf.wifi.sta.enable=true`.  Assignment of new values to
`Sys.conf` properties is not permament, on reboot device will re-read values
from configuation file. Function `Sys.conf.save(reboot)` saves changed
configuration to `conf.json` file.  If `reboot` parameter is set to `true`
(default value) device will be rebooted after save in orders to reinitialize
all Smart.js modules.
