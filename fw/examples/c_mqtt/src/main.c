#include <stdio.h>

#include "mgos_i2c.h"

#include "common/platform.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_sys_config.h"
#include "mgos_wifi.h"

#include "mgos_mqtt.h"

enum {
  ERROR_UNKNOWN_COMMAND = -1,
  ERROR_I2C_NOT_CONFIGURED = -2,
  ERROR_I2C_READ_LIMIT_EXCEEDED = -3
};

static void sub(struct mg_connection *c, const char *fmt, ...) {
  char buf[100];
  struct mg_mqtt_topic_expression te = {.topic = buf, .qos = 1};
  uint16_t sub_id = mgos_mqtt_get_packet_id();
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  mg_mqtt_subscribe(c, &te, 1, sub_id);
  LOG(LL_INFO, ("Subscribing to %s (id %u)", buf, sub_id));
}

static void pub(struct mg_connection *c, const char *fmt, ...) {
  char msg[200];
  struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = json_vprintf(&jmo, fmt, ap);
  va_end(ap);
  mg_mqtt_publish(c, mgos_sys_config_get_mqtt_pub(), mgos_mqtt_get_packet_id(),
                  MG_MQTT_QOS(1), msg, n);
  LOG(LL_INFO, ("%s -> %s", mgos_sys_config_get_mqtt_pub(), msg));
}

static uint8_t from_hex(const char *s) {
#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')
  int a = tolower(*(const unsigned char *) s);
  int b = tolower(*(const unsigned char *) (s + 1));
  return (HEXTOI(a) << 4) | HEXTOI(b);
}

static void gpio_int_handler(int pin, void *arg) {
  static double last = 0;
  double now = mg_time();
  if (now - last > 0.2) {
    struct mg_connection *c = mgos_mqtt_get_global_conn();
    last = now;
    if (c != NULL) {
      pub(c, "{type: %Q, pin: %d}", "click", pin);
    }
    LOG(LL_INFO, ("Click!"));
  }
  (void) arg;
}

static void ev_handler(struct mg_connection *c, int ev, void *p,
                       void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

  if (ev == MG_EV_MQTT_CONNACK) {
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    if (mgos_sys_config_get_mqtt_sub() == NULL ||
        mgos_sys_config_get_mqtt_pub() == NULL) {
      LOG(LL_ERROR, ("Run 'mgos config-set mqtt.sub=... mqtt.pub=...'"));
    } else {
      sub(c, "%s", mgos_sys_config_get_mqtt_sub());
    }
  } else if (ev == MG_EV_MQTT_SUBACK) {
    LOG(LL_INFO, ("Subscription %u acknowledged", msg->message_id));
  } else if (ev == MG_EV_MQTT_PUBLISH) {
    struct mg_str *s = &msg->payload;
    struct json_token t = JSON_INVALID_TOKEN;
    char buf[100], asciibuf[sizeof(buf) * 2 + 1];
    int i, pin, state, addr, len;

    LOG(LL_INFO, ("got command: [%.*s]", (int) s->len, s->p));
    /* Our subscription is at QoS 1, we must acknowledge messages sent ot us. */
    mg_mqtt_puback(c, msg->message_id);
    if (json_scanf(s->p, s->len, "{gpio: {pin: %d, state: %d}}", &pin,
                   &state) == 2) {
      /* Set GPIO pin to a given state */
      mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
      mgos_gpio_write(pin, (state > 0 ? 1 : 0));
      pub(c, "{type: %Q, pin: %d, state: %d}", "gpio", pin, state);
    } else if (json_scanf(s->p, s->len, "{button: {pin: %d}}", &pin) == 1) {
      /* Report button press on GPIO pin to a publish topic */
      mgos_gpio_set_button_handler(pin, MGOS_GPIO_PULL_UP,
                                   MGOS_GPIO_INT_EDGE_POS, 50, gpio_int_handler,
                                   NULL);
      pub(c, "{type: %Q, pin: %d}", "button", pin);
    } else if (json_scanf(s->p, s->len, "{i2c_read: {addr: %d, len: %d}}",
                          &addr, &len) == 2) {
      /* Read from I2C */
      struct mgos_i2c *i2c = mgos_i2c_get_global();
      if (len <= 0 || len > (int) sizeof(buf)) {
        pub(c, "{error: {code: %d, message: %Q}}",
            ERROR_I2C_READ_LIMIT_EXCEEDED, "Too long read");
      } else if (i2c == NULL) {
        pub(c, "{error: {code: %d, message: %Q}}", ERROR_I2C_NOT_CONFIGURED,
            "I2C is not enabled");
      } else {
        bool ret;
        asciibuf[0] = '\0';
        ret = mgos_i2c_read(i2c, addr, (uint8_t *) buf, len, true /* stop */);
        if (ret) {
          for (i = 0; i < len; i++) {
            const char *hex = "0123456789abcdef";
            asciibuf[i * 2] = hex[(((uint8_t *) buf)[i] >> 4) & 0xf];
            asciibuf[i * 2 + 1] = hex[((uint8_t *) buf)[i] & 0xf];
          }
          asciibuf[i * 2] = '\0';
        }
        pub(c, "{type: %Q, status: %d, data: %Q}", "i2c_read", ret, asciibuf);
      }
    } else if (json_scanf(s->p, s->len, "{i2c_write: {data: %T}}", &t) == 1) {
      /* Write byte sequence to I2C. First byte is the address */
      struct mgos_i2c *i2c = mgos_i2c_get_global();
      if (i2c == NULL) {
        pub(c, "{error: {code: %d, message: %Q}}", ERROR_I2C_NOT_CONFIGURED,
            "I2C is not enabled");
      } else {
        bool ret;
        int j = 0;
        for (int i = 0; i < t.len; i += 2, j++) {
          ((uint8_t *) t.ptr)[j] = from_hex(t.ptr + i);
        }
        ret = mgos_i2c_write(i2c, t.ptr[0], t.ptr + 1, j, true /* stop */);
        pub(c, "{type: %Q, status: %d}", "i2c_write", ret);
      }
    } else {
      pub(c, "{error: {code: %d, message: %Q}}", ERROR_UNKNOWN_COMMAND,
          "unknown command");
    }
  }
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_mqtt_add_global_handler(ev_handler, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
