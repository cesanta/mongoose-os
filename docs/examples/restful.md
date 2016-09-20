---
title: Device as a RESTful server
---

This example shows how to implement a RESTful server on a device.

Mongoose IoT firmware has
[Mongoose networking engine](https://github.com/cesanta/mongoose) inside.
It has HTTP, WebSocket, MQTT, CoAP functionality built-in, both client
and server. In this example, we'll use HTTP server functionality.

We'll implement a RESTful server that accepts two numbers on `/add` URI,
and replies with the sum of these two numbers.

We get the "system" mongoose instance via `mg_get_mgr()` call, then use
[Mongoose API](https://docs.cesanta.com/mongoose/master/) (and there is a
large set of
[examples](https://github.com/cesanta/mongoose/tree/master/examples)
to our disposal) to implement the functionality we want.

Here's what we write in `app.c`:

```c
#include &lt;stdio.h&gt;

#include "fw/src/mg_mongoose.h"

static void cb(struct clubby_request_info *ri, void *cb_arg,
               struct clubby_frame_info *fi, struct mg_str args) {
  int pin, state, result;

  // args is a JSON string with parameters. Try to fetch pin and state values
  if (json_scanf(args.p, (int) args.len, "{pin: %d, state: %d}", &pin,
                 &state) == 2) {
    // Success. Set the pin into the requested state.
    sj_gpio_set_mode(pin, GPIO_MODE_INOUT, GPIO_PULL_PULLUP);
    result = sj_gpio_write(pin, state);

    // Report the result
    clubby_send_responsef(ri, "%d", result);
  } else {
    // Error fetching pin and state - report error back.
    clubby_send_errorf(ri, 1, "%s", "Expeted: {pin: X, state: X}");
  }

  (void) cb_arg;
  (void) fi;
}

enum mg_app_init_result sj_app_init(void) {
  // Register a handler for the "/Set" command
  clubby_add_handler(mg_clubby_get_global(), mg_mk_str("/Set"), cb, NULL);
  return MG_APP_INIT_SUCCESS;
}
```

Compile and flash:

```
$ dev flash -port /dev/ttyS0 -fw build/fw.zip
```

Test is with `curl`:

```sh
$ curl -u USER:TOKEN https://DEVICE_ID.api.mongoose-iot.com/Set -d '{"pin":14, "state": 1}'
{"v":2,"id":1913050485526,"result":0}
```

We have set the GPIO pin 14 to "on" state.
If there is an LED attached to that pin, it turns on.
