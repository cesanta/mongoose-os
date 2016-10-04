---
title: miot tool
---

Mongoose IoT platform comes with a swiss army knife tool, called `miot`.
It is used to practically all tasks - building and flashing the firmware,
setting device configuration, managing files on a device,
performing over-the-air updates, managing devices on a cloud, and so on.

Download `miot` tool from https://mongoose-iot.com/downloads.html

`miot` is a powerful instrument to integrate device management into the
development process. For example, it is easy to build an IDE plugin
to list and edit files directly on a target device, or to update a selected
list of devices remotely - either configuration files, or a full firmware.

`miot` tool is self-documented. Running it without arguments shows a help
string:

```
$ miot
Error: command required

The Mongoose IoT command line tool. Usage:
  miot <command> ...


Commands:
  auth         Manage auth credentials for the Mongoose IoT
  cloud        Cloud operations
  dev          Work with local devices
  fw           Work with local firmware
```

To see the documentation for a certain command, type `miot COMMAND`, e.g.

```
$ miot dev fs
Error: command required

List, read or write files at the local device's filesystem. Usage:
  miot dev fs <command> ...


Commands:
  ls           List files at the local device's filesystem
  get          Read file from the local device's filesystem and print to stdout
  put          Put file from the host machine to the local device's filesystem
```
