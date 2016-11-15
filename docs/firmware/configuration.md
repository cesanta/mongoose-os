---
title: Configuration
---

# Overview

Mongoose Firmware uses a structured, multi-layer configuration.

It is defined by a JSON file and represented in C code by a struct and in JavaScript by an object. Integer, boolean and string values are supported.

The configuration can be extended by the user (new fields can be added).

At boot, values are initialised with defaults and can be overridden from two layers of the configuration files; vendor and user.

Values can be changed at runtime, but usually a reboot is required for new settings to be applied.

A web interface is provided to manage the configuration using a browser.

# Definition and representation

Configuration is defined by the JSON configuration contained in the [defaults file](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/fs/conf_sys_defaults.json).

## C

The object defined in `conf_sys_defaults.json` is translated into a C `struct sys_config`.
Types are inferred from the default values.

For example, this fragment
```JSON
{
  "wifi": {
    "ap": {
      "enable": true,
      "trigger_on_gpio": -1,
      "ssid": "Mongoose_??????"
    }
  }
}
```
will result in the following struct (generated at build time):
```C
struct sys_config {
  struct sys_config_wifi {
    struct sys_config_wifi_ap {
      int enable;
      int trigger_on_gpio;
      char *ssid;
    } ap;
  } wifi;
};
```
Numbers are represented by integers, as are booleans. Strings will be allocated on the heap.

*Important note*: Empty strings will be represented as `NULL` pointers, be careful.

To get access to the config values, include `fw/src/miot_sys_config.h`.
One global instance of `struct sys_config` is created during boot and is returned by `get_cfg()`.
Setting values is possible, but usually a reboot is required for changes to take effect.
`miot_conf_set_str()` function should be used to set string values.

Configuration can be saved by invoking `save_cfg()`.

## JavaScript

In JavaScript, the `Sys.conf` object provides access to the config values.
Note that new fields cannot be added at runtime. Trying to add a new field will throw an exception.
Existing values can be changed, but as a rule, a reboot is required for changes to take effect.
See the "Extensibility" section below for how to add your own fields.

_Note_: Setting whole config sections (objects) is not supported. Fields must be set one at a time.

The `Sys.conf.save()` method will save the configuration and reboot the device.

# Boot time configuration

At boot time, `struct sys_config` is intialised in the following order:

 - First, the struct is zeroed.
 - Second, defaults from `conf_sys_defaults.json` are applied.
 - Third, if it exists, the _vendor configuration file_, `conf_vendor.json`, is applied.
 - The _user configuration file_, `conf.json`, is applied on as the last step.

The result is the state of the global `struct sys_config`. Each step (layer) can override some, all or none of the values.
Defaults must be loaded and it is an error if the file does not exist at the time of boot. But, vendor and user layers are optional.

# Web interface

Mongoose Firmware ships with a web server that lets the user change the configuration values.
Annotations for the web interface come from the [schema file](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/fs/conf_sys_schema.json).

# Resetting to factory defaults

If configured by `debug.factory_reset_gpio`, holding the specified pin low during boot will wipe out user settings (`conf.json`).
Note, vendor settings, if present, are not reset.
This function is also available through the web interface.

# Vendor configuration

A vendor configuration layer exists to facilitate post-production configuration: devices can be customised by uploading a single file
(e.g. via HTTP POST to `/upload`) instead of performing a full reflash.
Vendor configuration is not reset by the "factory reset", whether via GPIO or web.

# Field access control

Some settings in the configuration may be sensitive and the vendor may, while providing a way for user to change settings,
restrict certain fields or (better) specify which fields can be changed by the user.
To facilitate that, the configuration system contains field access control, configured by the `field access control list` (ACL).

 - ACL is a comma-delimited list of entries which are applied to full field names when loading config files at boot time.
 - ACL entries are matched in order and, search terminates when a match is found.
 - ACL entry is a pattern, where `*` serves as a wildcard.
 - ACL entry can start with `+` or `-`, specifying whether to allow or deny change to the field if the entry matches. `+` is implied but can be used for clarity.
 - The default value of the ACL is `*`, meaning changing any field is allowed.

ACL is contained in the configuration itself - it's the top-level `conf_acl` field.
The slight twist is that during loading, the setting of the _previous_ layer is in effect: when loading user settings, `conf_acl` from vendor settings is consulted,
and for vendor settings the `conf_acl` value from the defaults is used.

## Examples

To restrict users to only being able change WiFi and debug level settings, `"conf_acl": "wifi.*,debug.level"` should be set in `conf_vendor.json`.

Negative entries allow for default-allow behaviour: `"conf_acl": "-debug.*,*"` allows changing all fields except anything under `debug`.

# Extensibility

Applications using Mongoose Firmware can add their own fields to `struct sys_config` and the corresponding `Sys.conf` JavaScript object.

During build, field definitions from [conf_sys_defaults.json](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/fs/conf_sys_defaults.json) are augmented with
definitions from `conf_app_defaults.json`. While empty by default, a different file can be supplied by the app developer by defining the `APP_CONF_DEFAULTS` variable,
as shown in the `c_hello` example [here](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/Makefile.build#L5).
The [file](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/fs/conf_app_defaults.json) defines a new string field, `hello.who`,
which is then [used](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/src/app_main.c#L18) to produce the greeting.
