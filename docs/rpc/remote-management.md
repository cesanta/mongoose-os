---
title: Remote management
---

Since an RPC call could be triggered via any transport, not just serial
connection, it is possible to control remote devices - and the level of this
control is incredible.

Example: scan I2C bus and return addresses of I2C peripherals from a device
at IP address 192.168.1.4 using `mos` tool in a command line mode:

```
$ mos --port ws://192.168.1.4/rpc call I2C.Scan
[
  31
]
```

Example: connect to a remote device via the MQTT server and manage it
using `mos` tool Web UI:

![](media/mos3.gif)