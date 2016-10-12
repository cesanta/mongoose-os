---
title: Custom C/C++ logic
---

Please see examples on how to customise Mongoose Firmware in C/C++:

- [c_hello](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples/c_hello) - most basic, "Hello World" example
- [c_network](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples/c_network) -  shows how to use the networking API to receive commands and send requests

These examples can be built from a command line or pasted into the
[cloud IDE](https://mongoose-iot.com) and built in a single click.

During the firmware boot process, C code creates an instance of our
[V7 JavaScript engine](https://github.com/cesanta/v7/),
`struct v7 *v7`, initialises the JS API and then executes the `sys_init.js`
file, effectively passing control to the JavaScript environment. Prior to
calling `sys_init.js`, C code calls the `int mg_app_init(struct v7 *)` function,
which should be defined by the user. In this function, it is possible to export
custom JavaScript objects and make them available to the JavaScript environment.

`mg_app_init()` function will return `MG_APP_INIT_SUCCESS` on successful
initialisation or `MG_APP_INIT_ERROR` on failure.


Also, see
- [V7 reference on exporting C/C++ functions to
  JS](https://docs.cesanta.com/v7/#_call_c_c_function_from_javascript)
- `init_v7()` function at
  [v7_esp.c](https://github.com/cesanta/mongoose-iot/blob/master/fw/platforms/esp8266/user/v7_esp.c)
  for an example of how the specific C/C++ API is exported to ESP8266 firmware. To
  extend it, just edit the `init_v7()` function and rebuild the firmare by running
  `make` in `fw/platforms/esp8266` directory.
- Firmware C API section below
