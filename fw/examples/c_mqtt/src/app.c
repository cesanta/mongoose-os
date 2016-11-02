#include <stdio.h>
#include <stdlib.h>

#include "common/platform.h"
#include "fw/src/mg_app.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_wifi.h"
#include "fw/src/mg_sys_config.h"
#include "frozen/frozen.h"
#include "fw/src/mg_gpio.h"

#if CS_PLATFORM == CS_P_ESP8266
#define GPIO_MAX 16
#elif CS_PLATFORM == CS_P_CC3200
#define GPIO_MAX 64
#else
#error Unknown platform
#endif

static char *s_topic = NULL;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;

  switch (ev) {
    case MG_EV_CONNECT: {
      struct mg_send_mqtt_handshake_opts opts;

      if (*(int *) ev_data != 0) {
        LOG(LL_ERROR,
            ("Failed to connect to MQTT broker (%s)", get_cfg()->mqtt.broker));
        break;
      }

      LOG(LL_INFO, ("Connected to broker, sending MQTT handshake"));
      memset(&opts, 0, sizeof(opts));
      opts.user_name = get_cfg()->device.id;
      opts.password = get_cfg()->device.password;

      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake_opt(nc, get_cfg()->device.id, opts);

      break;
    }
    case MG_EV_MQTT_CONNACK: {
      struct mg_mqtt_topic_expression topic_expression;

      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
        LOG(LL_ERROR,
            ("Got MQTT connection error: %d\n", msg->connack_ret_code));
        break;
      }

      LOG(LL_INFO, ("MQTT handshake OK"));

      topic_expression.topic = s_topic;
      topic_expression.qos = 0;

      LOG(LL_INFO, ("Subscribing to %s", s_topic));

      mg_mqtt_subscribe(nc, &topic_expression, 1, 1);

      break;
    }
    case MG_EV_MQTT_SUBACK: {
      LOG(LL_INFO, ("Subscription acknowledged"));
      break;
    }
    case MG_EV_MQTT_PUBLISH: {
      int gpio_pin, gpio_state;

      LOG(LL_INFO, ("Got incoming message %.*s: %.*s\n", (int) msg->topic.len,
                    msg->topic.p, (int) msg->payload.len, msg->payload.p));

      if (strncmp(msg->topic.p, s_topic, msg->topic.len) != 0) {
        LOG(LL_WARN,
            ("Unexpected topic %.*s", (int) msg->topic.len, msg->topic.p));
        break;
      }

      if (json_scanf(msg->payload.p, msg->payload.len,
                     "{gpio_pin: %d, state: %d}", &gpio_pin, &gpio_state) < 0) {
        LOG(LL_ERROR, ("Failed to parse payload\r\n"));
        break;
      }

      if ((gpio_pin < 0 || gpio_pin > GPIO_MAX) ||
          (gpio_state != 0 && gpio_state != 1)) {
        LOG(LL_ERROR, ("Invalid arguments"));
        break;
      }

      LOG(LL_INFO, ("Set gpio %d to %d", gpio_pin, gpio_state));

      mg_gpio_set_mode(gpio_pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
      mg_gpio_write(gpio_pin, (enum gpio_level) gpio_state);

      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("Conection to broker closed"));
      break;
    }
  }
}

static void on_wifi_change(enum mg_wifi_status event, void *ud) {
  (void) ud;

  switch (event) {
    case MG_WIFI_IP_ACQUIRED: {
      if (mg_connect(mg_get_mgr(), get_cfg()->mqtt.broker, ev_handler) ==
          NULL) {
        LOG(LL_ERROR,
            ("Failed to connect to broker (%s)", get_cfg()->mqtt.broker));
      }
      break;
    }
    case MG_WIFI_DISCONNECTED: {
      LOG(LL_WARN, ("Disconnected from WiFi"));
      break;
    }
    case MG_WIFI_CONNECTED: {
      LOG(LL_INFO, ("Connected to WiFi"));
    };
    default:
      ;
  }
}

enum mg_app_init_result mg_app_init(void) {
  LOG(LL_INFO, ("MQTT client started"));

  mg_asprintf(&s_topic, 0, "/control/%s", get_cfg()->device.id);
  mg_wifi_add_on_change_cb(on_wifi_change, NULL);

  return MG_APP_INIT_SUCCESS;
}
