---
title: Extending Mongoose IoT firmware
---

It is trivial to add more API functions to the Mongoose IoT firmware.  Mongoose IoT is
built on top of [V7 JavaScript engine](https://github.com/cesanta/v7/) which
makes it easy to export C/C++ functions to JavaScript:

- [V7 reference on exporting C/C++ functions to
  JS](https://docs.cesanta.com/v7/#_call_c_c_function_from_javascript)
- See `init_v7()` function at
  [v7_esp.c](https://github.com/cesanta/iot/blob/master/platforms/esp8266/user/v7_esp.c)
  for an example of how specific C/C++ API is exported to ESP8266 firmware. To
  extend it, just edit `init_v7()` function and rebuild the firmare by running
  `make` in `fw/platforms/esp8266` directory.
