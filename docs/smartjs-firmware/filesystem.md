---
title: Filesystem
---

Smart.js uses SPIFFS filesystem on some of the boards (e.g. ESP8266, CC3200).
SPIFFS is a flat filesystem, i.e. it has no directories. To provide the same
look at feel on all platforms, Smart.js uses flat filesystem on all
architectures.

Below there is a quick description of the files and their meaning.  System
files, not supposed to be edited:

- `sys_init.js`: Main system initialization file. This file is the only file
  called by the C firmware code.
- `conf_sys_defaults.json`: System configuration.
- `conf_sys_schema.json`: Contains description of the system configuration,
  used by the Web UI to render controls.
- `conf.json`: This file can be absent. It is created by the Web UI when user
  saves configuration, and contains only overrides to system and app config
  files.  NOTE: this file is preserved during OTA (Over-The-Air firmware
  update).
- `index.html`: Configuration Web UI file.
- `sys_*.js`: Various drivers.
- `imp_*`: Files with `imp_` prefix are preserved during OTA update. Thus, if
  you'd like some data to survive firmware update, place that data into a file
  with prefix `imp_`.

Files that are meant to be edited by developers:

- `app.js`: Application-specific file. This file is called by `sys_init.js`.
  User code must go here.
- `conf_app_defaults.json`: Application-specific configuration file. Initially
  empty.  If application wants to show it's own config parameters on the
  configuration Web UI, those parameters should go in this file.
- `conf_app_schema.json`: Description of the app-specific config options.
