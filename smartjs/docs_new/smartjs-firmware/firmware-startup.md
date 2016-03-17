---
title: Firmware startup process
---

When Smart.js starts, it reads `conf_sys_defaults.json`, merges it with,
`conf_app_defaults.json`, and subsequently merges with `conf.json`.
Therefore, in order to override any setting from the default config files,
put the override into `conf.json` - that's exactly what Web UI is doing when
user presses Save button.

`conf_app_defaults.json` file is empty by default. Put your application
specific configuration parameters there, the same way system parameters are
kept in `conf_sys_defaults.json`.

When firmware starts, it automatically connects to the cloud, which provides
services like OTA updates, device registry, time-series database,
PubSub (publish-subscribe), etc. That could be switched off in a respective
setting in the configuration file (`clubby.connect_on_boot`),
programmatically or using the Web UI.
