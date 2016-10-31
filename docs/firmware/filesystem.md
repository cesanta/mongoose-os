---
title: Filesystem
---

Mongoose IoT Platform uses a SPIFFS filesystem on some of the boards (e.g. ESP8266, CC3200).
SPIFFS is a flat filesystem, i.e. it has no directories. To provide the same
look at feel on all platforms, Mongoose IoT Platform uses a flat filesystem on all
architectures.

Below, there is a description of the files and their meaning.  System
files (not supposed to be edited):

- `conf_sys_defaults.json`: These are system configuration parameters. Can be
  overridden in `conf.json`.
- `conf_sys_schema.json`: This contains a description of the system configuration,
  used by the Web UI to render the configuration page.
- `conf.json`: This file can be absent. It is created
  when a user calls `Sys.conf.save()` function or by the Web UI when a user
  saves configuration. `conf.json` contains only overrides to the system
  and the app config files.
- `index.html`: Configuration Web UI file.

Files that are meant to be edited by developers:

- `conf_app_defaults.json`: Application-specific configuration file. Initially
  empty.  If the application wants to show it's own config parameters on the
  configuration Web UI, those parameters should go in this file.
- `conf_app_schema.json`: Description of the app-specific config options.
