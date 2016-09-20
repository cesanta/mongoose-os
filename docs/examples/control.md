---
title: Remote device control
---

This example shows how connected device can be remotely controlled via
mobile phone, web console, etc.

The idea is that the device creates a persistent connection to the cloud.
A mobile device send a command to the cloud, saying
"send this data to the device with ID XXXX". Cloud verifies access rights
and passes on the command to the device.

See Quick Start guide on how to build a firmware, flash a new device,
and register it on the cloud. Once the device is registered, it is
accessible via https://DEVICE_ID.api.mongoose-iot.com URL.
That endpoint accepts control RESTful POST requests encoded as JSON
messages (we call these JSON messages "clubby frames". Clubby format
is described in the Cloud section of this documentation).

In order for the device to react on the control command, let's write some
command handler code. Let's make the device switch some equipment on or off.
Let's call the command `/Set`, and pass two parameters to it - a GPIO pin
number and a state, 0 or 1.

Here's what we write in `app.c`:

```c
#include &lt;stdio.h&gt;

#include "common/clubby/clubby.h"
#include "frozen/frozen.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_gpio.h"

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

And now, let's sent the command remotely via curl. First, go to the cloud UI
and click on the "Auth" tab. There, copy the auth token. Enter in the console:

```sh
$ curl -u USER:TOKEN https://DEVICE_ID.api.mongoose-iot.com/Set -d '{"pin":14, "state": 1}'
{"v":2,"id":1913050485526,"result":0}
```

We have set the GPIO pin 14 to "on" state.
If there is an LED attached to that pin, it turns on.
