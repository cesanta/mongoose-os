#include <stdio.h>

#include "common/platform.h"
#include "fw/src/mg_app.h"
#include "fw/src/mg_gpio.h"
#include "fw/src/mg_mqtt.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_wifi.h"

static void sub(struct mg_connection *c, const char *fmt, ...) {
  char buf[100];
  struct mg_mqtt_topic_expression topic = {buf, 0};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  mg_mqtt_subscribe(c, &topic, 1, 42);
  LOG(LL_INFO, ("Subscribed to %s", buf));
}

static void ev_handler(struct mg_connection *c, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

  if (ev == MG_EV_MQTT_CONNACK) {
    /* Connected to the cloud, subscribe to the control topic */
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    sub(c, "/%s/gpio", get_cfg()->device.id);
  } else if (ev == MG_EV_MQTT_PUBLISH) {
    /* Received control command - parse it and apply the logic */
    struct mg_str *s = &msg->payload;
    int pin, state;
    if (json_scanf(s->p, s->len, "{pin: %d, state: %d}", &pin, &state) == 2) {
      mg_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
      mg_gpio_write(pin, state > 0 ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
      LOG(LL_INFO, ("Done: [%.*s]", (int) s->len, s->p));
    } else {
      LOG(LL_ERROR, ("Unknown command: [%.*s]", (int) s->len, s->p));
    }
  }
}

static void on_wifi_event(enum mg_wifi_status ev, void *data) {
  if (ev == MG_WIFI_IP_ACQUIRED) {
    mg_mqtt_set_global_handler(ev_handler, NULL);
  }
  (void) data;
}

enum mg_app_init_result mg_app_init(void) {
  mg_wifi_add_on_change_cb(on_wifi_event, 0);
  return MG_APP_INIT_SUCCESS;
}
