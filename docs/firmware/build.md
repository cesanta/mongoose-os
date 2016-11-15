---
title: Custom C/C++ logic
---

Please see
[examples](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples)
on how to customise Mongoose Firmware in C/C++.

`miot_app_init()` function will return `MIOT_APP_INIT_SUCCESS` on successful
initialisation or `MIOT_APP_INIT_ERROR` on failure.

Also, see
- [V7 reference on exporting C/C++ functions to
  JS](https://docs.cesanta.com/v7/#_call_c_c_function_from_javascript)
- `init_v7()` function at
  [v7_esp.c](https://github.com/cesanta/mongoose-iot/blob/master/fw/platforms/esp8266/user/v7_esp.c)
  for an example of how the specific C/C++ API is exported to ESP8266 firmware. To
  extend it, just edit the `init_v7()` function and rebuild the firmare by running
  `make` in `fw/platforms/esp8266` directory.
- Firmware C API section below
