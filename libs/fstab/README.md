# Storage device and filesystem init table

## Overview

Configures storage devices, creates and/or mounts filesystems according to configuration.

## Configuration

This library defines two configurtion sections: `devtab` (5 entries) and `fstab` (3 entries).

Devtab defines VFS devices to be created, fstab defines filesystems to create and/or mount.

Devtab is processed first, then fstab.

See `config_schema` section of the [manifest](mos.yml) for detailed description of settings.

## Examples

 * External SPI flash formatted as LFS and mounted on `/data`. Size is auto-detected.

```yaml
libs:
  - origin: https://github.com/cesanta/mongoose-os/libs/fstab
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-dev-spi-flash
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-fs-lfs

config_schema:
  - ["spi.enable", true]
  # Other SPI interface options go here.
  - ["devtab.dev0.name", "spif0"]
  - ["devtab.dev0.type", "spi_flash"]
  - ["devtab.dev0.opts", '{"cs": 0, "freq": 10000000}']
  - ["fstab.fs0.dev", "spif0"]
  - ["fstab.fs0.type", "LFS"]
  - ["fstab.fs0.opts", '{"bs": 4096}']
  - ["fstab.fs0.path", "/data"]
  - ["fstab.fs0.create", true]
```

 * (ESP32) Additional partition on the ESP32 system flash, formatted as LFS and mounted on `/data`.

_Note:_ All the data ESP32 partitions are automatically registered so there are no explicit devtab entries.

```yaml
libs:
  - origin: https://github.com/cesanta/mongoose-os/libs/fstab
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-fs-lfs

build_vars:
  ESP_IDF_EXTRA_PARTITION: data,data,spiffs,,256K

config_schema:
  - ["fstab.fs0.dev", "data"]
  - ["fstab.fs0.type", "LFS"]
  - ["fstab.fs0.opts", '{"bs": 4096}']
  - ["fstab.fs0.path", "/data"]
  - ["fstab.fs0.create", true]
```

 * External SPI flash split into two parts

First formatted for SPIFFS, the rest is not used.

```yaml
libs:
  - origin: https://github.com/cesanta/mongoose-os/libs/fstab
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-dev-part
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-dev-spi-flash
  - origin: https://github.com/cesanta/mongoose-os/libs/vfs-fs-spiffs

config_schema:
  - ["spi.enable", true]
  # Other SPI interface options go here.
  - ["devtab.dev0.name", "spif0"]
  - ["devtab.dev0.type", "spi_flash"]
  - ["devtab.dev0.opts", '{"cs": 0, "freq": 10000000}']
  - ["devtab.dev1.name", "spif0p1"]
  - ["devtab.dev1.type", "part"]
  - ["devtab.dev1.opts", '{"dev": "spif0", "offset": 0, "size": 131072}']
  - ["devtab.dev2.name", "spif0p2"]
  - ["devtab.dev2.type", "part"]
  - ["devtab.dev2.opts", '{"dev": "spif0", "offset": 131072}']
  - ["fstab.fs0.dev", "spif0p1"]
  - ["fstab.fs0.type", "SPIFFS"]
  - ["fstab.fs0.opts", '{"bs": 4096, "ps": 128, "es": 4096}']
  - ["fstab.fs0.path", "/data"]
  - ["fstab.fs0.create", true]
```
