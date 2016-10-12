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

Create an empty directory, execute `miot fw init --arch cc3200` in it.
If you have NodeMCU, use `esp8266` instead of `cc3200`.
Copy-paste this into the `src/app.c`:

```c
#include "common/cs_dbg.h"
#include "frozen/frozen.h"
#include "fw/src/mg_app.h"
#include "fw/src/mg_mongoose.h"

static void add_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *) ev_data;

    /* Get form variables n1 and n2, and compute their sum */
    char n1[100], n2[100], reply[100];
    mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
    mg_get_http_var(&hm->body, "n2", n2, sizeof(n2));
    double result = strtod(n1, NULL) + strtod(n2, NULL);

    /* Send JSON-formatted reply */
    struct json_out out = JSON_OUT_BUF(reply, sizeof(reply));
    json_printf(&out, "{result: %d}", (int) result);

    mg_printf(c, "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/json\r\n\r\n%s\n", reply);

    /* Close the connection */
    c->flags |= MG_F_SEND_AND_CLOSE;
  }
}

enum mg_app_init_result mg_app_init(void) {
  mg_register_http_endpoint(mg_get_http_listening_conn(), "/add", add_handler);
  return MG_APP_INIT_SUCCESS;
}
```

Attach a device to your computer. Compile, flash and configure the firmware:

```
$ miot cloud build
$ miot dev flash --fw build/fw.zip --port /dev/ttyUSB0  # Your TTY device could be different
$ miot dev config set --port /dev/ttyUSB0 wifi.sta.enable=true wifi.sta.ssid=WIFI_NETWORK wifi.sta.pass=WIFI_PASSWORD
```

Find out the device's IP address:

```sh
$ miot dev call --port /dev/ttyUSB0 /v1/Config.GetNetworkStatus
{
  "wifi": {
    "sta_ip": "IP_ADDRESS",
    "ap_ip": "192.168.4.1",
    "status": "got ip",
    "ssid": "WIFI_NETWORK"
  }
}
```

Now test with `curl`:

```sh
$ curl -d 'n1=1&n2=2' http://IP_ADDRESS/add
{"result": 3}
```
