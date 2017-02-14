---
title: Compile time generation
---

Configuration is defined by several YAML files in the Mongoose OS source
repository. Each Mongoose OS module, for example, crypto chip support module,
can define it's own section in the configuration. Here are few examples:

- [mgos_sys_config.yaml](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_sys_config.yaml) is core module, defines debug settings, etc
- [mgos_atca_config.yaml](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_atca_config.yaml) is a crypto chip support module
- [mgos_mqtt_config.yaml](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_mqtt_config.yaml) has default MQTT server settings

There are more, you can see them all at
[fw/src](https://github.com/cesanta/mongoose-os/tree/master/fw/src) directory.

As has been mentioned in the overview, you can define your own sections in
the config, or override existing default values. This is done by placing
another YAML file in the `YOUR_FIRMWARE_DIR/src` directory and specifying
`APP_CONF_SCHEMA` variable in the `mos.yml`.
Take a look at
[fw/examples/c_hello](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_hello/src) for an example: there is custom `src/conf_schema.yaml` file, and
`mos.yml` has this snippet:

```yaml
build_vars:
  APP_CONF_SCHEMA: src/conf_schema.yaml
```

When the firmware is built, all these YAML files get merged into one.
User-specified YAML file goes last, therefore it can override any other.
Then, merged YAML file gets translated into two C files, `sys_config.h` and
and `sys_config.c`. You can find these generated files in the
`YOUR_FIRMWARE_DIR/build/gen/` directory after you build your firmware.

`sys_config.h` contains configuration structure definition, and prototypes
for `get_cfg()` and `save_cfg()` functions. `sys_config.c` contains the
implementation.

Here's a translation example, taken from
[fw/examples/c_hello](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_hello).
There, we have a custom `src/conf_schema.yaml`:

```yaml
[
  ["hello", "o", {"title": "Hello app settings"}],
  ["hello.who", "s", "world", {"title": "Who to say hello to"}]
]
```

It gets translated into the following C structure:

```c
struct sys_config {
 ...
 struct sys_config_hello {
   char *who;
 } hello;
};
```

Then, C firmware code in [src/main.c](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_hello/src/main.c) accesses that custom configuration value:

```c
  printf("Hello, %s!\n", get_cfg()->hello.who);
```

Numbers are represented by integers, as are booleans.
Strings will be allocated on the heap.

**IMPORTANT NOTE**: Empty strings will be represented as `NULL` pointers,
be careful.
