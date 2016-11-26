#include <stdio.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_mqtt.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

#if CS_PLATFORM == CS_P_ESP8266
#define BUTTON_PIN 0 /* GPIO0, "Flash" button on NodeMCU */
#elif CS_PLATFORM == CS_P_CC3200
#define BUTTON_PIN 15 /* SW2 on LAUNCHXL */
#else
#error Unknown platform
#endif

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
      miot_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
      miot_gpio_write(pin, state > 0 ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
      LOG(LL_INFO, ("Done: [%.*s]", (int) s->len, s->p));
    } else {
      LOG(LL_ERROR, ("Unknown command: [%.*s]", (int) s->len, s->p));
    }
  }
}

static void on_wifi_event(enum miot_wifi_status ev, void *data) {
  if (ev == MIOT_WIFI_IP_ACQUIRED) {
    miot_mqtt_set_global_handler(ev_handler, NULL);
  }
  (void) data;
}

void gpio_int_handler(int pin, enum gpio_level level, void *arg) {
  static double last = 0;
  double now = mg_time();
  if (now - last > 0.2) {
    struct mg_connection *nc = miot_mqtt_get_global_conn();
    last = now;
    if (nc != NULL) {
      char message[50], topic[50];
      struct json_out jmo = JSON_OUT_BUF(message, sizeof(message));
      int n = json_printf(&jmo, "{pin: %d}", pin);
      snprintf(topic, sizeof(topic), "/%s/button", get_cfg()->device.id);
      mg_mqtt_publish(nc, topic, 123, MG_MQTT_QOS(0), message, n);
      LOG(LL_INFO, ("Click! Published %s to %s.", message, topic));
    } else {
      LOG(LL_INFO, ("Click!"));
    }
  }
  miot_gpio_intr_set(BUTTON_PIN, GPIO_INTR_POSEDGE);
  (void) level;
  (void) arg;
}

enum miot_app_init_result miot_app_init(void) {
  miot_wifi_add_on_change_cb(on_wifi_event, 0);
  miot_gpio_intr_init(gpio_int_handler, NULL);
  miot_gpio_intr_set(BUTTON_PIN, GPIO_INTR_POSEDGE);
  return MIOT_APP_INIT_SUCCESS;
}
