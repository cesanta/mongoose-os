---
title: Overview
---


Mongoose OS uses a structured, multi-layer configuration.
It consists of two parts: a compile time part that defines configuration,
and a run time part that uses configuration.

#### Compile-time configuration generation:

- Any piece of code that requires config setting, can define a .yaml file
that describes configuration parameters for that piece of code
- User code could define it's own set of configuration parameters in
it's own .yaml file
- All these yaml files are merged together during firmware compilation,
and a single `sys_config.json` file is generated
- User-defined YAML file is applied last, therefore it can override any
default settings specified in the system .yaml file
- Generated `sys_config.json` file represents all possible
  configurable settings for the firmware
- A C header and source files are also generated. C header contains a structure
that mirrors `sys_config.json` file, and an API for getting and setting
individual configuration values

<center>
![compile-time config](media/config1.png)
</center>

#### Run-time 3-layer configuration intitialisation:

- `conf_defaults.json` - configuration defaults. This is a replica of generated
  `sys_config.json`.
- `vendor.json` vendor overrides. Absent by default. If present,
   gets applied on top of the defaults.
- `conf.json` user overrides. Absent by default. If present, gets applied on top
   of the defaults + vendor overrides. All `mos config-set` config changes
   modify `conf.json`.

<center>
![run-time config](media/config2.png)
</center>

#### Therefore here are the rules of thumb:

- If you need to define your own config parameters, create your yaml
  description file ([see example](https://github.com/cesanta/mongoose-os/blob/master/fw/skeleton/src/conf_schema.yaml)), and specify its location in `mos.yaml`
  ([see example](https://github.com/cesanta/mongoose-os/blob/master/fw/skeleton/mos.yml))
- If you want to override some system default setting, for example
  a default UART speed, do it in the same user YAML file -- see previous point.
- If you want to put some unique information on each firmware, for example
  a unique ID, and optionally protect it from further modification, use `vendor.json` file.
- All other overrides to system defaults (e.g. WiFi settings) go into `conf.json`.
  

So, firmware configuration is defined by a set of YAML description files,
which get translated into a C structure during firmware build.
C code can access configuration
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
