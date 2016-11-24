#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "frozen/frozen.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_mongoose.h"

static void ctl_handler(struct mg_connection *c, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *) p;
    struct mg_str *s = hm->body.len > 0 ? &hm->body : &hm->query_string;

    int pin, state, status = -1;
    if (json_scanf(s->p, s->len, "{pin: %d, state: %d}", &pin, &state) == 2) {
      miot_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
      miot_gpio_write(pin, state > 0 ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
      status = 0;
    }
    mg_printf(c, "HTTP/1.0 200 OK\n\n{\"status\": %d}\n", status);
    c->flags |= MG_F_SEND_AND_CLOSE;
    LOG(LL_INFO, ("Got: [%.*s]", (int) s->len, s->p));
  }
}

enum miot_app_init_result miot_app_init(void) {
  miot_register_http_endpoint("/ctl", ctl_handler);
  return MIOT_APP_INIT_SUCCESS;
}
