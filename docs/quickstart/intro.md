---
title: Quick Start Guide
items:
  - { name: build.md }
  - { name: flash.md }
  - { name: configure.md }
  - { name: register.md }
  - { name: label.md }
  - { name: update.md }
---

For the most impatient, this is a 1-minute overview of the following
quick start guide:

```
$ miot fw init --arch esp8266   # Or, --arch cc3200
$ miot cloud build
$ miot dev flash -port /dev/ttyUSB0 -fw build/fw.zip
$ miot dev config set -port /dev/ttyUSB0 \
  wifi.ap.enable=false \
  wifi.sta.enable=true \
  wifi.sta.ssid=WIFI_NETWORK_NAME \
  wifi.sta.pass=WIFI_PASSWORD
$ miot dev register -port /dev/ttyUSB0
```

For those who wo\uld like to have more explanations, read along.
