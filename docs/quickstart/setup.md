---
title: Setup mos tool
---

Mongoose OS uses a command line utility `mos` for various tasks:
installation (flashing firmware), building firmware, managing files on
a device, calling device's services, and so on.
Download and install `mos` tool following instructions
at https://mongoose-os.com/software.html.

`mos` should be invoked this way: `mos COMMAND OPTIONAL_FLAGS`.
To see what commands are available, run `mos --help`. To see help for a
specific command, run `mos help COMMAND`. If `mos` is run without any
command, either from a terminal or by double-clicking the executable,
`mos` starts a simple Web UI, handy for a quick installation and running
examples quickly.

By default, `mos` tool connects to the device via the serial port. Which
exactly serial port is used, could be specified by the `--port PORT` flag,
e.g. `--port COM3` on Windows or `--port /dev/ttyUSB0` on Linux. By default,
`--port auto` is used, which makes `mos` enumerate all serial devices
on your system and pick the first suitable.

You might need to install a USB-to-Serial driver on your OS:

- [FTDI drivers](http://www.ftdichip.com/Drivers/VCP.htm) for CC3200
- [Silabds drivers](https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx) for Espressif boards

It is possible to set `--port` value to be a network endpoint instead of
serial port. Device listens for commands on serial, Websocket, and MQTT
transports (unless they are disabled). Therefore, `--port ws://IP_ADDR/rpc`
connects to the remote device via Websocket, and
`--port mqtt://MQTT_SERVER/DEVICE_ID/rpc` via the MQTT protocol.
That gives an ability to use `mos` tool as a remote device management tool.

The default values for any `mos` flag could be overridden via the
environment variable `MOS_FLAGNAME`. For example, to set the default value
for serial port, export `MOS_PORT` variable - on Mac/Linux,
put that into your `~/.profile`:

```
export MOS_PORT=YOUR_SERIAL_PORT  # E.g. /dev/ttyUSB0
```
