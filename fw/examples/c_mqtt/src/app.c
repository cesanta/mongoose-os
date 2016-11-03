#include <stdio.h>

#include "common/platform.h"
#include "fw/src/mg_app.h"
#include "fw/src/mg_gpio.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_mqtt.h"

static struct mg_mqtt_topic_expression topic_expressions[] = {{"/stuff", 0}};

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  (void) nc;

#if 0
  if (ev != MG_EV_POLL)
    printf("USER HANDLER GOT %d\n", ev);
#endif

  switch (ev) {
    case MG_EV_MQTT_CONNACK:
      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
        printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
      }
      printf("Subscribing to '/stuff'\n");
      mg_mqtt_subscribe(nc, topic_expressions,
                        sizeof(topic_expressions) / sizeof(*topic_expressions),
                        42);
      break;
    case MG_EV_MQTT_PUBACK:
      printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
      break;
    case MG_EV_MQTT_SUBACK:
      printf("Subscription acknowledged, forwarding to '/test'\n");
      break;
    case MG_EV_MQTT_PUBLISH: {
      printf("Got incoming message %.*s: %.*s\n", (int) msg->topic.len,
             msg->topic.p, (int) msg->payload.len, msg->payload.p);

      if (mg_vcmp(&msg->topic, "/stuff") == 0) {
        printf("Forwarding to /test\n");
        mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), msg->payload.p,
                        msg->payload.len);
      } else {
        printf("Ignoring\n");
      }
      break;
    }
  }
}

enum mg_app_init_result mg_app_init(void) {
  mg_mqtt_set_global_handler(ev_handler, NULL);

  return MG_APP_INIT_SUCCESS;
}
