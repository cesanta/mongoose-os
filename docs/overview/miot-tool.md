---
title: miot tool
---

Mongoose IoT platform comes with a swiss army knife tool, called `miot`.
It is used to build and flash the firmware,
set device configuration, manage files on a device,
and register devices on Mongoose Cloud.

Download `miot` tool from https://mongoose-iot.com/software.html
On Linux and Mac, make it executable after the download:

```bash
cp miot ~/bin/
chmod 755 ~/bin/miot
```

`miot` is a powerful instrument to integrate device management into the
development process. For example, it is easy to build an IDE plugin
to list and edit files directly on a target device.

`miot` tool is self-documented. Running it without arguments shows a help
string:

```
The Mongoose IoT command line tool. Usage:
  miot <command>

Commands:
  init             Initialise firmware directory structure in the current directory
  build            Build a firmware from the sources located in the current directory and download resulting fw.zip file
  flash            Flash firmware to the device
  console          Simple serial port console
  ls               List files at the local device's filesystem
  get              Read file from the local device's filesystem and print to stdout
  put              Put file from the host machine to the local device's filesystem
  register-device  Set id/psk to the device and register it at the cloud
  config-get       Get config value from the locally attached device
  config-set       Set config value at the locally attached device
```

Hints:

- Run `miot help <command>` to see which flags are understood by the command
- If you want to avoid specifying certain command flags all the time,
  consider setting environment variables `MIOT_FLAGNAME`. Then `miot`
  will fetch default flag value from the environment variable:

```bash
export MIOT_PORT=/dev/ttyUSB0
export MIOT_USER=joe
export MIOT_PASS=mypass
```
