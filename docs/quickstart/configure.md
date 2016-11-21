---
title: Configure WiFi
---

The device is flashed, but not configured yet.
Let's set up WiFi networking on it:

```bash
miot config-set wifi.ap.enable=false wifi.sta.enable=true \
  wifi.sta.ssid=WIFI_NETWORK_NAME wifi.sta.pass=WIFI_PASSWORD
```

This command alters a user configuration file on the device's filesystem
and reboots the device. After the reboot, a device joins the WiFi network.
