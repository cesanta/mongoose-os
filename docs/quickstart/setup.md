---
title: Setup mos tool
---

Mongoose OS uses `mos` tool for various tasks:
installation (flashing firmware), building firmware from C sources,
managing files on
a device, calling device's RPC services, and so on.
Download and install `mos` tool following instructions
at https://mongoose-os.com/software.html:



`mos` should be invoked this way: `mos COMMAND OPTIONAL_FLAGS`.
To see what commands are available, run `mos --help`:

```
$ mos --help
The Mongoose OS command line tool, v. 20170430-122307/master@4277964b. Usage:
  mos &lt;command&gt;

Commands:
  ui             Start GUI
  init           Initialise firmware directory structure in the current directory
  build          Build a firmware from the sources located in the current directory
  flash          Flash firmware to the device
  flash-read     Read a region of flash
  console        Simple serial port console
  ls             List files at the local device's filesystem
  get            Read file from the local device's filesystem and print to stdout
  put            Put file from the host machine to the local device's filesystem
  rm             Delete a file from the device's filesystem
  config-get     Get config value from the locally attached device
  config-set     Set config value at the locally attached device
  call           Perform a device API call. "mos call RPC.List" shows available methods
  aws-iot-setup  Provision the device for AWS IoT cloud
  update         Self-update mos tool
  wifi           Setup WiFi - shortcut to config-set wifi...

Global Flags:
  --verbose      Verbose output. Optional, default value: "false"
  --logtostderr  log to standard error instead of files. Optional, default value: "false"

```

If `mos` is run without any
command, either from a terminal or by double-clicking the executable,
`mos` starts a simple Web UI, handy for a quick installation and running
examples:

![](media/mos1.gif)


`mos` tool connects to the device specified by `--port` flag, which is
set to `auto` by default. That means, `mos` auto-detects the serial port
for the device. You can specify this value manually. It could be a
serial device,  e.g. `--port COM3` on Windows or `--port /dev/ttyUSB0` on Linux.

You might need to install a USB-to-Serial driver on your OS:

- [FTDI drivers](http://www.ftdichip.com/Drivers/VCP.htm) for CC3200
- [Silabs drivers](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx) for Espressif boards

It is possible to set `--port` value to be a network endpoint instead of
serial port. Device listens for commands on serial, Websocket, and MQTT
transports (unless they are disabled). Therefore, `--port ws://IP_ADDR/rpc`
connects to the remote device via Websocket, and
`--port mqtt://MQTT_SERVER/DEVICE_ID/rpc` via the MQTT protocol.
That gives an ability to use `mos` tool as a remote device management tool.

The default values for any `mos` flag could be overridden via the
environment variable `MOS_FLAGNAME`. For example, to set the default value
for `--port` flag, export `MOS_PORT` variable - on Mac/Linux,
put that into your `~/.profile`:

```
export MOS_PORT=YOUR_SERIAL_PORT  # E.g. /dev/ttyUSB0
```
