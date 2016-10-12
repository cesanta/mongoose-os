---
title: Register the device
---

The device is flashed, and configured, it can talk to the outside world.
Let's register it with the cloud:

```shell
$ miot dev register -port /dev/ttyUSB0
```

This command asks a cloud for the new device ID/password, register that
ID/password under your credentials, and writes ID/password to the device's
configuration.

When powered, a device connects to the cloud via SSL and
authenticates with ID/password. From that point on, it is possible to send
commands to the device by talking to the cloud, since cloud will redirect
all commands to the connected device after the proper access control checks.
