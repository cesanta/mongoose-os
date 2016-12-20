#include "common/cs_dbg.h"
#include "frozen/frozen.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_wifi.h"

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;
  int pin, state, status = 0;

  if (ev == MG_EV_HTTP_REQUEST) {
    struct mg_serve_http_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.document_root = "/";
    mg_serve_http(c, ev_data, opts);
    return;
  }

  if (ev != MG_EV_WEBSOCKET_FRAME) return;

  LOG(LL_INFO, ("[%.*s]", (int) wm->size, wm->data));
  if (json_scanf((char *) wm->data, wm->size, "{pin: %d, state: %d}", &pin,
                 &state) == 2) {
    miot_gpio_set_mode(pin, MIOT_GPIO_MODE_OUTPUT);
    miot_gpio_write(pin, state > 0 ? 1 : 0);
  } else {
    status = 1; /* Error */
  }
  mg_printf_websocket_frame(c, WEBSOCKET_OP_TEXT, "{\"status\": %d}", status);
}

enum miot_app_init_result miot_app_init(void) {
  struct mg_connection *c;
  struct mg_bind_opts bind_opts;
  const char *err;

  memset(&bind_opts, 0, sizeof(bind_opts));
  bind_opts.ssl_cert = "server.pem";
  bind_opts.ssl_key = "server.key";
  bind_opts.error_string = &err;

  c = mg_bind_opt(miot_get_mgr(), "443", ev_handler, bind_opts);
  if (c == NULL) {
    LOG(LL_ERROR, ("FAIL: %s", err));
    return MIOT_APP_INIT_ERROR;
  } else {
    mg_set_protocol_http_websocket(c);
  }

  return MIOT_APP_INIT_SUCCESS;
}
