---
title: WiFi
---

The most common use case of the WiFi interface is to install an event handler
to catch the moment when Mongoose Firmware joins the WiFi network and to kick
off the networking logic from that point on:

```c
    static void on_wifi_changed(enum sj_wifi_status status) {
      switch (status) {
        case SJ_WIFI_CONNECTED:
          printf("wifi connected");
          break;
        case SJ_WIFI_DISCONNECTED:
          printf("wifi disconnected");
          break;
        case SJ_WIFI_IP_ACQUIRED:
          printf("wifi IP: %s", sj_wifi_get_sta_ip());
          break;
      }
    }

    int sj_app_init(struct v7 *v7) {
      ...
      sj_wifi_set_on_change_cb(on_wifi_changed);
      ...
    }
```

See [C header file](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/sj_wifi.h)
for more information.
