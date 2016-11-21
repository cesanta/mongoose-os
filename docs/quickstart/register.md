---
title: Register device
---

The device is flashed, and configured, it can talk to the outside world.
Let's register it with the Mongoose Cloud:

```
miot register
```

This command generates ID and password for the device, register that
device under your cloud account, and writes ID/password to the device's
configuration. Generated ID is `ARCHITECTURE_MACADDRESS`, for example
`esp8266_4f12ea`. Generated password is random. It is possible to specify
device ID and password manually via `--device-id` and `--device-pass` flags.

If a device with given ID is already registered, the command will fail.
You can remove the device on the cloud, than register again. Note that
removing the device will erase all data that has been reported by that device.

When powered, a device connects to the cloud and
authenticates with ID/password. From that point on, it is possible to control
device remotely.
