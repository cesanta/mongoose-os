---
title: Quick Start Guide
items:
  - { name: build.md }
  - { name: flash.md }
  - { name: configure.md }
  - { name: register.md }
  - { name: control.md }
---

For the most impatient, this is a 1-minute overview of the following
quick start guide:

```
$ miot fw init --arch esp8266
$ miot cloud build
$ miot dev flash -port /dev/ttyS0 -fw build/fw.zip
$ miot dev config set -wifi.ssid WIFI_NETWORK_NAME -wifi.pass WIFI_PASSWORD
$ miot dev register
```

For those who would like to have more explanations, read along.
