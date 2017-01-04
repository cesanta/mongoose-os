#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "frozen/frozen.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_mongoose.h"

static void ctl_handler(struct mg_connection *c, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {
    struct http_message *hm = (struct http_message *) p;
    struct mg_str *s = hm->body.len > 0 ? &hm->body : &hm->query_string;

    int pin, state, status = -1;
    if (json_scanf(s->p, s->len, "{pin: %d, state: %d}", &pin, &state) == 2) {
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
      mgos_gpio_write(pin, state);
      status = 0;
    }
    mg_printf(c, "HTTP/1.0 200 OK\n\n{\"status\": %d}\n", status);
    c->flags |= MG_F_SEND_AND_CLOSE;
    LOG(LL_INFO, ("Got: [%.*s]", (int) s->len, s->p));
  }
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_register_http_endpoint("/ctl", ctl_handler);
  return MGOS_APP_INIT_SUCCESS;
}
