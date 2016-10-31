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

Flags:
      --arch string          Hardware architecture. Possible values: esp8266, cc3200
      --baud-rate uint       Serial port speed (default 115200)
      --device-id string     Device ID
      --device-pass string   Device pass/key
      --firmware string      Firmware .zip file location (file of HTTP URL) (default "build/fw.zip")
      --helpfull             Show full help, including advanced flags
      --pass string          Cloud password or token
      --port string          Serial port where the device is connected
      --server string        Cloud server (default "http://cloud.mongoose-iot.com")
      --timeout duration     Timeout for the device connection (default 10s)
      --user string          Cloud username
      --version              Print miot tool version and exit
```
