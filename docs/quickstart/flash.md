---
title: Flash the firmware
---

Now it's time to flash a built firmware.
Get one of the
[supported hardware modules](#/overview/hardware.md/),
for example TI CC3200 or ESP8266-based NodeMCU.
Connect a hardware module to your computer and run the following command:

```bash
miot flash -port /dev/ttyUSB0 -fw build/fw.zip
```

Note that the serial port on your system may be different.

Hint: do `export MIOT_PORT=/dev/ttyUSB0` environment variable, and it will
be used by the `miot` tool if you don't specify `--port` flag. That works
for any other flag. You can put commonly used flags (`MIOT_USER`, `MIOT_PASS`,
`MIOT_PORT`, `MIOT_FW`) into your `~/.profile` and do just `miot build`,
`miot flash`, etc.
