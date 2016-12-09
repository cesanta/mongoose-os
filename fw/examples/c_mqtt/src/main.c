#include <stdio.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_i2c.h"
#include "fw/src/miot_mqtt.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

enum { ERROR_UNKNOWN_COMMAND = -1, ERROR_I2C_NOT_CONFIGURED = -2 };

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

static void pub(struct mg_connection *c, const char *fmt, ...) {
  char msg[200];
  struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = json_vprintf(&jmo, fmt, ap);
  va_end(ap);
  mg_mqtt_publish(c, get_cfg()->mqtt.pub, 123, MG_MQTT_QOS(0), msg, n);
  LOG(LL_INFO, ("%s -> %s", get_cfg()->mqtt.pub, msg));
}

static uint8_t from_hex(const char *s) {
#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')
  int a = tolower(*(const unsigned char *) s);
  int b = tolower(*(const unsigned char *) (s + 1));
  return (HEXTOI(a) << 4) | HEXTOI(b);
}

void gpio_int_handler(int pin, enum gpio_level level, void *arg) {
  static double last = 0;
  double now = mg_time();
  if (now - last > 0.2) {
    struct mg_connection *c = miot_mqtt_get_global_conn();
    last = now;
    if (c != NULL) {
      pub(c, "{type: %Q, pin: %d}", "click", pin);
    }
    LOG(LL_INFO, ("Click!"));
  }
  miot_gpio_intr_set(pin, GPIO_INTR_POSEDGE);
  (void) level;
  (void) arg;
}

static void ev_handler(struct mg_connection *c, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

  if (ev == MG_EV_MQTT_CONNACK) {
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    if (get_cfg()->mqtt.sub == NULL || get_cfg()->mqtt.pub == NULL) {
      LOG(LL_ERROR, ("Run 'miot config-set mqtt.sub=... mqtt.pub=...'"));
    } else {
      sub(c, "%s", get_cfg()->mqtt.sub);
    }
  } else if (ev == MG_EV_MQTT_PUBLISH) {
    struct mg_str *s = &msg->payload;
    int pin, state;
    char *data = NULL;

    LOG(LL_INFO, ("got command: [%.*s]", (int) s->len, s->p));
    if (json_scanf(s->p, s->len, "{gpio: {pin: %d, state: %d}}", &pin,
                   &state) == 2) {
      /* Set GPIO pin to a given state */
      miot_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
      miot_gpio_write(pin, state > 0 ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
      pub(c, "{type: %Q, pin: %d, state: %d}", "gpio", pin, state);
    } else if (json_scanf(s->p, s->len, "{button: {pin: %d}}", &pin) == 1) {
      /* Report button press on GPIO pin to a publish topic */
      miot_gpio_intr_init(gpio_int_handler, NULL);
      miot_gpio_intr_set(pin, GPIO_INTR_POSEDGE);
      pub(c, "{type: %Q, pin: %d}", "button", pin);
    } else if (json_scanf(s->p, s->len, "{i2c_write: {data: %Q}}", &data) ==
               1) {
      /* Write byte sequence to I2C. First byte is an address */
      struct miot_i2c *i2c = miot_i2c_get_global();
      if (i2c == NULL) {
        pub(c, "{error: {code: %d, message: %Q}}", ERROR_I2C_NOT_CONFIGURED,
            "I2C is not enabled");
      } else {
        enum i2c_ack_type ret = miot_i2c_start(i2c, from_hex(data), I2C_WRITE);
        for (size_t i = 2; i < strlen(data) && ret == I2C_ACK; i += 2) {
          ret = miot_i2c_send_byte(i2c, from_hex(data + i));
          LOG(LL_DEBUG, ("i2c -> %02x", from_hex(data + i)));
        }
        miot_i2c_stop(i2c);
        pub(c, "{type: %Q, result: %d}", "i2c", ret);
      }
    } else {
      pub(c, "{error: {code: %d, message: %Q}}", ERROR_UNKNOWN_COMMAND,
          "unknown command");
    }
    free(data);
  }
}

static void on_wifi_event(enum miot_wifi_status ev, void *data) {
  if (ev == MIOT_WIFI_IP_ACQUIRED) {
    miot_mqtt_set_global_handler(ev_handler, NULL);
  }
  (void) data;
}

enum miot_app_init_result miot_app_init(void) {
  miot_wifi_add_on_change_cb(on_wifi_event, 0);
  return MIOT_APP_INIT_SUCCESS;
}
