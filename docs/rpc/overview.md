---
title: Overview
---

Mongoose OS WiFi provides a Remote Procedure Call API that makes device
accessible remotely. The API allows to:

- Create an RPC service. In a nutshell, this is C function that is given
  an RPC endpoint name, and gets triggered when a request for that
  endpoint arrives in one of the supported channels (serial, HTTP, MQTT, etc)
- Call RPC service. Allows to call a given endpoint remotely - for example,
  another device

Mongoose OS RPC requests (MG-RPC) and responses are marshalled into JSON frames,
which are very similar to JSON-RPC. The difference is that MG-RPC adds
some extra optional keys.

For an example, take a look at
[c_rpc](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_rpc)
example firmware that implements `Example.Increment` RPC service.

Mongoose OS command line tool provides an easy way to call device's RPC
services, over the serial connection or over the network. For example,

Get a list of all RPC services implemented by a device attached to a serial port:

```
$ mos call RPC.List
Using port /dev/cu.SLAB_USBtoUART
[
  "I2C.WriteRegW",
  "I2C.WriteRegB",
  "I2C.ReadRegW",
  "I2C.ReadRegB",
  ...
```

Scan I2C bus and return addresses of I2C peripherals from a device
at IP address 192.168.1.4:

```
$ mos --port ws://192.168.1.4/rpc call I2C.Scan
[
  31
]
```
