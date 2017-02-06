/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_rpc_channel_mqtt.h"

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mqtt.h"

static char *mgos_rpc_mqtt_topic_name(const struct mg_str device_id) {
  char *topic = NULL;
  mg_asprintf(&topic, 0, "%.*s/rpc", (int) device_id.len,
              device_id.p ? device_id.p : "");
  return topic;
}

static void mg_rpc_mqtt_handler(struct mg_connection *nc, int ev,
                                void *ev_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) nc->user_data;
  switch (ev) {
    case MG_EV_MQTT_SUBACK:
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      break;
    case MG_EV_MQTT_PUBLISH:
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &msg->payload);
      break;
    case MG_EV_CLOSE:
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      break;
  }
}

static void mg_rpc_channel_mqtt_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void frame_sent(void *arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
}

static bool mg_rpc_channel_mqtt_send_frame(struct mg_rpc_channel *ch,
                                           const struct mg_str f) {
  struct mg_connection *nc = mgos_mqtt_get_global_conn();
  if (nc != NULL) {
    struct json_token dst;
    char *topic = NULL;
    if (json_scanf(f.p, f.len, "{dst:%T}", &dst) != 1) {
      LOG(LL_ERROR,
          ("Cannot reply to RPC over MQTT, no dst: [%.*s]", (int) f.len, f.p));
      return false;
    }
    topic = mgos_rpc_mqtt_topic_name(mg_mk_str_n(dst.ptr, dst.len));
    mg_mqtt_publish(nc, topic, 0, MG_MQTT_QOS(1), f.p, f.len);
    LOG(LL_DEBUG, ("Published [%.*s] to topic [%s]", (int) f.len, f.p, topic));
    free(topic);
    mgos_invoke_cb(frame_sent, ch);
    return true;
  }
  return false;
}

static void mg_rpc_channel_mqtt_ch_close(struct mg_rpc_channel *ch) {
  (void) ch;
}

static const char *mg_rpc_channel_mqtt_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "MQTT";
}

static bool mg_rpc_channel_mqtt_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  return true;
}

struct mg_rpc_channel *mg_rpc_channel_mqtt(const struct mg_str device_id) {
  char *topic = mgos_rpc_mqtt_topic_name(device_id);
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_mqtt_ch_connect;
  ch->send_frame = mg_rpc_channel_mqtt_send_frame;
  ch->ch_close = mg_rpc_channel_mqtt_ch_close;
  ch->get_type = mg_rpc_channel_mqtt_get_type;
  ch->is_persistent = mg_rpc_channel_mqtt_is_persistent;
  mgos_mqtt_global_subscribe(mg_mk_str(topic), mg_rpc_mqtt_handler, ch);
  LOG(LL_INFO, ("%p %s", ch, topic));
  free(topic);
  return ch;
}
