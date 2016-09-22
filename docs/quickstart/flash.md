---
title: Flash the firmware
---

Now it's time to flash a built firmware.
Get one of the supported hardware modules, for example TI CC3200 or
ESP8266-based NodeMCU. Connect a hardware module to your computer and
run the following command:

```shell
$ dev flash -port /dev/ttyUSB0 -fw build/fw.zip
```

Note that the serial port on your system may be different.
