---
title: Overview
---

Mongoose Firmware uses a structured, multi-layer configuration.
It is defined by a YAML description file, which gets translated into
a C structure during firmware build. C code can access configuration
parameters by directly accessing fields in the generated structure.
Fields can be integer, boolean or string. C functions to retrieve and save
that global configuration object are generated.

Example on how to access configuration parameters:

```c
  struct sys_config *cfg = get_cfg();
  printf("My device ID is: %d\n", cfg->device.id);  // Get config param
```

Example on how to set configuration parameter and save the configuration:
```c
  struct sys_config *cfg = get_cfg();
  cfg->device.passwod = "big secret!";
  char *err = NULL;
  save_cfg(cfg, &err);
  printf("Saving configuration: %s\n", err ? err : "no error");
  free(err);
```

The generation mechanism not only gives a handy C API, but also guarantees
that if the C code accesses some parameter, it is indeed in the description
file and thus is meant to be in the firmware. That protects from the common
problems when the configuration is refactored/changed, but C code left intact.

Mongoose OS configuration is extensible, i.e. it is possible to add your own
configuration parameters, which might be either simple, or complex (nested).

At run time, a configuration is backed by several files on a filesystem.
It has 3 layers: factory defaults, vendor overrides, and user overrides.
Vendor layer can "lock" certain parts of
configuration for the user layer, and allow only certain fields to be changed.
For example, end-user might change the WiFi settings, but cannot change the
address of the cloud backend.


<!--
the two C files: `buid/gen/sys_config.h` and `build/gen/sys_config.c`.
A header file contains

You can see the generated structure


during the
firmware build
 and represented in C code by a structure


and in JavaScript by an object. Integer, boolean and string values are supported.

The configuration can be extended by the user (new fields can be added).

At boot, values are initialised with defaults and can be overridden from two layers of the configuration files; vendor and user.

Values can be changed at runtime, but usually a reboot is required for new settings to be applied.

See example on how to build a web UI for managing device
configuration at
https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_web_config .
-->
