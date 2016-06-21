---
title: Custom C/C++ logic
---

Please see examples on how to customize Mongoose firmware in C/C++:

- [c_hello](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples/c_hello) - most basic, "Hello World" example
- [c_network](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples/c_network) -  shows how to use networking API to receive commands and send requests

These examples could be built from a command line, or pasted into the
[cloud IDE](https://mongoose-iot.com) and built in a single click.

During firmware boot process, C code creates an instance of
[V7 JavaScript engine](https://github.com/cesanta/v7/),
`struct v7 *v7`, initializes JS API and then executes `sys_init.js`
file, effectively passing control to the JavaScript environment. Prior to
calling `sys_init.js`, C code calls `int sj_app_init(struct v7 *)` function,
which should be defined by user. In this function, it is possible to export
custom JavaScript objects and make them available to the JavaScript environment.

`sj_app_init()` function shall return `MG_APP_INIT_SUCCESS` on successful
initialization, or `MG_APP_INIT_ERROR` on failure.


Also, see
- [V7 reference on exporting C/C++ functions to
  JS](https://docs.cesanta.com/v7/#_call_c_c_function_from_javascript)
- `init_v7()` function at
  [v7_esp.c](https://github.com/cesanta/mongoose-iot/blob/master/fw/platforms/esp8266/user/v7_esp.c)
  for an example of how specific C/C++ API is exported to ESP8266 firmware. To
  extend it, just edit `init_v7()` function and rebuild the firmare by running
  `make` in `fw/platforms/esp8266` directory.
- Firmware C API section below
