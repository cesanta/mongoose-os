/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "v7/v7.h"
#include "mongoose/mongoose.h"
#include "smartjs/src/sj_hal.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_common.h"

#define SJ_MQTT_CONNECT_CB "_cocb"
#define SJ_MQTT_MESSAGE_CB "_mecb"
#define SJ_MQTT_ERROR_CB "_ercb"
#define SJ_MQTT_CLOSE_CB "_clcb"

struct user_data {
  struct v7 *v7;
  uint32_t msgid; /* next message id */
  v7_val_t client;
  char *client_id;
};

static void mqtt_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  struct v7 *v7 = ud->v7;
  v7_val_t cb;
  (void) nc;
  (void) ev_data;

  switch (ev) {
    case MG_EV_CONNECT:
      if (*(int *) ev_data == 0) {
        mg_send_mqtt_handshake(nc, ud->client_id);
      } else {
        cb = v7_get(v7, ud->client, SJ_MQTT_ERROR_CB, ~0);
        if (!v7_is_undefined(cb)) {
          sj_invoke_cb0(v7, cb);
        }
      }
      break;
    case MG_EV_MQTT_CONNACK: {
      /*
       * Call connect or error cb if they were registered.
       */
      const char *key;
      if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_ACCEPTED) {
        key = SJ_MQTT_CONNECT_CB;
      } else {
        key = SJ_MQTT_ERROR_CB;
      }
      cb = v7_get(v7, ud->client, key, ~0);
      if (!v7_is_undefined(cb)) {
        sj_invoke_cb0(v7, cb);
      }
      break;
    }
    case MG_EV_MQTT_PUBLISH:
      cb = v7_get(v7, ud->client, SJ_MQTT_MESSAGE_CB, ~0);

      if (!v7_is_undefined(cb)) {
        v7_val_t topic = v7_mk_string(v7, msg->topic, strlen(msg->topic), 1);
        v7_val_t payload =
            v7_mk_string(v7, msg->payload.p, msg->payload.len, 1);
        sj_invoke_cb2(v7, cb, topic, payload);
      }
      break;
    case MG_EV_CLOSE:
      /*
       * Invoke close cb and then destroys all mg state.
       */
      cb = v7_get(v7, ud->client, SJ_MQTT_CLOSE_CB, ~0);
      if (!v7_is_undefined(cb)) {
        sj_invoke_cb0(v7, cb);
      }

      v7_def(v7, ud->client, "_nc", ~0, _V7_DESC_HIDDEN(1), v7_mk_undefined());
      v7_disown(v7, &ud->client);
      free(ud->client_id);
      free(ud);
      break;
  }
}

/*
 * Make a new MQTT client.
 *
 * Arguments:
 * url: url where to connect to
 * opts: option object
 *
 * Recognized option object properties:
 *
 * - clientId: string; mqtt client id. defaults to
 *             Math.random().toString(16).substr(2, 10)
 *
 * Example:
 *
 * var client = MQTT.connect('mqtt://test.mosquitto.org');
 *
 * client.on('connect', function () {
 *   client.subscribe('presence');
 *   client.publish('presence', 'Hello mqtt');
 * });
 *
 * client.on('message', function (topic, message) {
 *   console.log(message);
 * });
 *
 * TLS can be enabled by choosing the `mqtts://` protocol. In that
 * case the default port is 8883 as defined by IANA.
 *
 * The API is modeled after https://www.npmjs.com/package/mqtt.
 *
 */
enum v7_err sj_mqtt_connect(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  const char *url;
  size_t len;
  struct mg_str host, scheme;
  unsigned int port;
  struct mg_connection *nc;
  struct user_data *ud;
  char *url_with_port = NULL;
  int use_ssl = 0;
  v7_val_t urlv = v7_arg(v7, 0), opts = v7_arg(v7, 1);
  v7_val_t client_id;
  v7_val_t proto =
      v7_get(v7, v7_get(v7, v7_get_global(v7), "MQTT", ~0), "proto", ~0);

  if (!v7_is_string(urlv)) {
    rcode = v7_throwf(v7, "Error", "invalid url string");
    goto clean;
  }

  url = v7_get_string_data(v7, &urlv, &len);

  if (mg_parse_uri(mg_mk_str(url), &scheme, NULL, &host, &port, NULL, NULL,
                   NULL) < 0) {
    rcode = v7_throwf(v7, "Error", "invalid url string");
    goto clean;
  }

  if (mg_vcmp(&scheme, "mqtt") == 0) {
    url += sizeof("mqtt://") - 1;
  } else if (mg_vcmp(&scheme, "mqtts") == 0) {
    url += sizeof("mqtts://") - 1;
    use_ssl = 1;
  } else {
    rcode = v7_throwf(v7, "Error", "unsupported protocol");
    goto clean;
  }

  client_id = v7_get(v7, opts, "clientId", ~0);
  if (v7_is_undefined(client_id)) {
    rcode = v7_exec(v7, "Math.random().toString(16).substr(2,8)", &client_id);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  if (port == 0) {
    int ret = asprintf(&url_with_port, "%.*s%s", (int) host.len, host.p,
                       (use_ssl ? ":8883" : ":1883"));
    (void) ret;
  }

  nc =
      mg_connect(&sj_mgr, url_with_port ? url_with_port : url, mqtt_ev_handler);
  if (nc == NULL) {
    rcode = v7_throwf(v7, "Error", "cannot create connection");
    goto clean;
  }

  if (use_ssl) {
#ifdef MG_ENABLE_SSL
    mg_set_ssl(nc, NULL, NULL);
#else
    rcode = v7_throwf(v7, "Error", "SSL not enabled");
    goto clean;
#endif
  }
  mg_set_protocol_mqtt(nc);

  *res = v7_mk_object(v7);
  v7_set_proto(v7, *res, proto);

  ud = calloc(1, sizeof(*ud));
  ud->v7 = v7;
  ud->client = *res;
  ud->client_id = strdup(v7_to_cstring(v7, &client_id));
  nc->user_data = ud;
  v7_own(v7, &ud->client);

  v7_def(v7, *res, "_nc", ~0, _V7_DESC_HIDDEN(1), v7_mk_foreign(nc));

clean:
  free(url_with_port);

  return rcode;
}

/*
 * Publishes a message to a topic.
 *
 * Args:
 * - `topic`: topic, string.
 * - `message`: message, string.
 *
 * Only QOS 0 is implemented at the moment.
 */
enum v7_err MQTT_publish(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct user_data *ud;
  struct mg_connection *nc;
  const char *topic;
  const char *message;
  size_t message_len;
  v7_val_t topicv = v7_arg(v7, 0), messagev = v7_arg(v7, 1);
  (void) res;

  topic = v7_to_cstring(v7, &topicv);
  if (topic == NULL || strlen(topic) == 0) {
    rcode = v7_throwf(v7, "TypeError", "invalid topic");
    goto clean;
  }

  if (!v7_is_string(messagev)) {
    rcode = v7_throwf(v7, "TypeError", "invalid message");
    goto clean;
  }
  message = v7_get_string_data(v7, &messagev, &message_len);

  nc = v7_to_foreign(v7_get(v7, v7_get_this(v7), "_nc", ~0));
  if (nc == NULL) {
    rcode = v7_throwf(v7, "Error", "invalid connection");
    goto clean;
  }
  ud = (struct user_data *) nc->user_data;

  mg_mqtt_publish(nc, topic, ud->msgid++, MG_MQTT_QOS(0), message, message_len);

clean:
  return rcode;
}

/*
 * Subscribes a mqtt client to a topic.
 */
enum v7_err MQTT_subscribe(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct user_data *ud;
  struct mg_connection *nc;
  struct mg_mqtt_topic_expression expr;
  v7_val_t topicv = v7_arg(v7, 0);
  const char *topic;

  nc = v7_to_foreign(v7_get(v7, v7_get_this(v7), "_nc", ~0));
  if (nc == NULL) {
    rcode = v7_throwf(v7, "Error", "unsupported protocol");
    goto clean;
  }
  ud = (struct user_data *) nc->user_data;

  topic = v7_to_cstring(v7, &topicv);
  if (topic == NULL || strlen(topic) == 0) {
    rcode = v7_throwf(v7, "TypeError", "invalid topic");
    goto clean;
  }

  expr.topic = topic;
  expr.qos = 0;
  mg_mqtt_subscribe(nc, &expr, 1, ud->msgid++);

  *res = v7_mk_boolean(1);
clean:
  return rcode;
}

/*
 * `on` method on a mqtt client object registers an event handler
 * for a given event type. Recognized event types:
 *
 * - `connect`: invoked on sucessfull connection (mqtt connack message)
 * - `error`: invoked on connection error
 * - `close`: invoked when the connection gets closed
 * - `message`: invoked when a new message is received. The callback
 *              receives (topic, message) arguments, both strings
 */
enum v7_err MQTT_on(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t evv = v7_arg(v7, 0);
  v7_val_t cb = v7_arg(v7, 1);
  const char *ev, *key = NULL;

  ev = v7_to_cstring(v7, &evv);
  if (strcmp(ev, "connect") == 0) {
    key = SJ_MQTT_CONNECT_CB;
  } else if (strcmp(ev, "message") == 0) {
    key = SJ_MQTT_MESSAGE_CB;
  } else if (strcmp(ev, "error") == 0) {
    key = SJ_MQTT_ERROR_CB;
  } else if (strcmp(ev, "close") == 0) {
    key = SJ_MQTT_CLOSE_CB;
  } else {
    rcode = v7_throwf(v7, "Error", "unsupported protocol");
    goto clean;
  }

  v7_def(v7, v7_get_this(v7), key, ~0, V7_DESC_ENUMERABLE(0), cb);

  (void) res;
clean:
  return rcode;
}

void sj_mqtt_api_setup(struct v7 *v7) {
  v7_val_t mqtt_proto = v7_mk_object(v7);
  v7_val_t mqtt_connect =
      v7_mk_function_with_proto(v7, sj_mqtt_connect, mqtt_proto);
  v7_val_t mqtt;
  v7_own(v7, &mqtt_connect);

  mqtt = v7_mk_object(v7);
  v7_own(v7, &mqtt);

  v7_set(v7, mqtt, "connect", ~0, mqtt_connect);
  v7_def(v7, mqtt, "proto", ~0, V7_DESC_ENUMERABLE(0), mqtt_proto);

  v7_set_method(v7, mqtt_proto, "publish", MQTT_publish);
  v7_set_method(v7, mqtt_proto, "subscribe", MQTT_subscribe);
  v7_set_method(v7, mqtt_proto, "on", MQTT_on);
  v7_set(v7, v7_get_global(v7), "MQTT", ~0, mqtt);

  v7_disown(v7, &mqtt);
  v7_disown(v7, &mqtt_connect);
}
