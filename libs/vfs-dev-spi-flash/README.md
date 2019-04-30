# SPI Flash VFS Driver

This driver supports external SPI flash chips and makes them avauilable for use with the mOS VFS subsystem.

It add an `spi_flash` device type.

Supported options (`dev_opts`) are:
 * `freq` - SPI bus frequency.
 * `cs` - SPI bus CS line to use, see the [SPI example](https://github.com/mongoose-os-apps/example-spi-c) for explanation.
 * `mode` - SPI mode, 0 - 2.
 * `size` - specify size explicitly. If not specified, an attempt is made to detect by querying SFDP data or JEDEC ID. Most modern chips support at least one of these, so specifying size is usually not necessary.
 * `wip_mask` - bit mask to apply to status register to determine when the write is in progress (chip is busy). Most chips have bit 0 as the `WIP` bit, and the corresponding mask value is `1`. This is the default.

Example of console output when this driver is used:

```
[Jul 28 14:35:20.697] mgos_vfs_dev_open    spi_flash ({"freq": 80000000, "cs": 0}) -> 0x3ffb47f0
[Jul 28 14:35:20.704] mgos_vfs_dev_spi_fla Chip ID: 20 71, size: 1048576
[Jul 28 14:35:20.714] mgos_vfs_mkfs        Create SPIFFS (dev 0x3ffb47f0, opts {"size": 262144, "force": true})
```
