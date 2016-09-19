---
title: Configure WiFi
---

The device is flashed, but not configured yet.
Let's set up WiFi networking on it:

```sh
$ miot dev config set -wifi.sta.enable=true \
                      -wifi.sta.ssid WIFI_NAME \
                      -wifi.sta.pass WIFI_PASS \
                      -port /dev/ttyS0
```

This command alters a user configuration file on the device's filesystem
and reboots the device. After the reboot, a device joins the WiFi network.
