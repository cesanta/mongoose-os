/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "mgos_rpc_channel_mqtt.h"
#include "mg_rpc.h"
#include "mgos_rpc.h"

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "frozen.h"
#include "mongoose.h"

#include "mgos_hal.h"
#include "mgos_sys_config.h"

#include "mgos_mqtt.h"

union mqtt_ch_data {
  uintptr_t v;
  struct {
    unsigned int open : 1;
    unsigned int sub_en : 1;
    unsigned int sub_acked : 1;
    unsigned int wc_sub_en : 1;
    unsigned int wc_sub_acked : 1;
    unsigned int sub_topic_len : 8;
  };
};

static struct mg_str mgos_rpc_mqtt_get_topic(const char *fmt,
                                             const struct mg_str arg,
                                             bool wildcard) {
  struct mg_str topic = MG_NULL_STR, topic2 = MG_NULL_STR;
  mg_asprintf((char **) &topic.p, 0, fmt, (int) arg.len, arg.p);
  if (wildcard && topic.p != NULL) {
    mg_asprintf((char **) &topic2.p, 0, "%s/#", topic.p);
    free((void *) topic.p);
    topic = topic2;
  }
  if (topic.p != NULL) topic.len = strlen(topic.p);
  return topic;
}

static void mgos_rpc_mqtt_sub_handler(struct mg_connection *nc, int ev,
                                      void *ev_data, void *user_data) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
  union mqtt_ch_data *chd = (union mqtt_ch_data *) &ch->channel_data;
  if (ev == MG_EV_MQTT_SUBACK) {
    if (chd->sub_en && !chd->sub_acked) {
      chd->sub_acked = true;
    } else if (chd->wc_sub_en && !chd->wc_sub_acked) {
      chd->wc_sub_acked = true;
    }
    if ((chd->sub_acked || !chd->sub_en) &&
        (chd->wc_sub_acked || !chd->wc_sub_en)) {
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
    }
    return;
  } else if (ev != MG_EV_MQTT_PUBLISH) {
    return;
  }
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
  if (chd->sub_topic_len == msg->topic.len) {
    ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &msg->payload);
  } else {
    struct mg_rpc_frame frame;
    /* Parse frame and ignore errors: they will be handled afterwards */
    mg_rpc_parse_frame(msg->payload, &frame);
    /* Replace method with the one from the topic name */
    frame.method =
        mg_mk_str_n(msg->topic.p + chd->sub_topic_len + 1 /* slash */,
                    msg->topic.len - chd->sub_topic_len - 1 /* slash */);
    ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, &frame);
  }
  (void) nc;
}

static void mgos_rpc_mqtt_handler(struct mg_connection *nc, int ev,
                                  void *ev_data, void *user_data) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
  union mqtt_ch_data *chd = (union mqtt_ch_data *) &ch->channel_data;
  if (ev == MG_EV_CLOSE && chd->open) {
    ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
    chd->open = false;
    chd->sub_acked = chd->wc_sub_acked = false;
  }
  (void) ev_data;
  (void) nc;
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
  if (nc == NULL) return false;
  const char *pub_topic_fmt = mgos_sys_config_get_rpc_mqtt_pub_topic();
  if (pub_topic_fmt == NULL) {
    LOG(LL_ERROR, ("pub_topic is not set, cannot send frame."));
    return false;
  }
  struct json_token dst = JSON_INVALID_TOKEN;
  if (strchr(pub_topic_fmt, '%') != NULL) {
    if (json_scanf(f.p, f.len, "{dst:%T}", &dst) != 1) {
      LOG(LL_ERROR,
          ("Cannot reply to RPC over MQTT, no dst: [%.*s]", (int) f.len, f.p));
      return false;
    }
  }
  struct mg_str topic =
      mgos_rpc_mqtt_get_topic(mgos_sys_config_get_rpc_mqtt_pub_topic(),
                              mg_mk_str_n(dst.ptr, dst.len), false);
  mg_mqtt_publish(nc, topic.p, mgos_mqtt_get_packet_id(),
                  MG_MQTT_QOS(mgos_sys_config_get_rpc_mqtt_qos()), f.p, f.len);
  LOG(LL_DEBUG, ("Published [%.*s] to topic [%s]", (int) f.len, f.p, topic.p));
  free((void *) topic.p);
  mgos_invoke_cb(frame_sent, ch, false /* from_isr */);
  return true;
}

static void mg_rpc_channel_mqtt_ch_close(struct mg_rpc_channel *ch) {
  /* TODO(rojer): Unsubscribe from topics */
  (void) ch;
}

static void mg_rpc_channel_mqtt_ch_destroy(struct mg_rpc_channel *ch) {
  free(ch);
}

static const char *mg_rpc_channel_mqtt_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "MQTT";
}

static bool mg_rpc_channel_mqtt_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static char *mg_rpc_channel_mqtt_get_info(struct mg_rpc_channel *ch) {
  (void) ch;
  return NULL;
}

struct mg_rpc_channel *mg_rpc_channel_mqtt(const struct mg_str device_id) {
  struct mg_str topic = mgos_rpc_mqtt_get_topic(
      mgos_sys_config_get_rpc_mqtt_sub_topic(), device_id, true);
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_mqtt_ch_connect;
  ch->send_frame = mg_rpc_channel_mqtt_send_frame;
  ch->ch_close = mg_rpc_channel_mqtt_ch_close;
  ch->ch_destroy = mg_rpc_channel_mqtt_ch_destroy;
  ch->get_type = mg_rpc_channel_mqtt_get_type;
  ch->is_persistent = mg_rpc_channel_true;
  /* Cannot broadcast, need specific destination */
  ch->is_broadcast_enabled = mg_rpc_channel_false;
  ch->get_authn_info = mg_rpc_channel_mqtt_get_authn_info;
  ch->get_info = mg_rpc_channel_mqtt_get_info;

  union mqtt_ch_data *chd = (union mqtt_ch_data *) &ch->channel_data;
  chd->v = 0;
  chd->sub_en = true;
  chd->sub_topic_len = topic.len - 2 /* /# */;

  mgos_mqtt_global_subscribe(mg_mk_str_n(topic.p, chd->sub_topic_len),
                             mgos_rpc_mqtt_sub_handler, ch);
  if (mgos_sys_config_get_rpc_mqtt_sub_wc()) {
    chd->wc_sub_en = true;
    mgos_mqtt_global_subscribe(topic, mgos_rpc_mqtt_sub_handler, ch);
  }
  /* For CLOSE event. */
  mgos_mqtt_add_global_handler(mgos_rpc_mqtt_handler, ch);

  LOG(LL_INFO, ("%p %.*s", ch, (int) chd->sub_topic_len, topic.p));
  free((void *) topic.p);
  return ch;
}

bool mgos_rpc_mqtt_init(void) {
  if (mgos_rpc_get_global() != NULL && mgos_sys_config_get_rpc_mqtt_enable()) {
    struct mg_rpc_channel *mch =
        mg_rpc_channel_mqtt(mg_mk_str(mgos_sys_config_get_device_id()));
    if (mch == NULL) return MGOS_INIT_MG_RPC_FAILED;
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(MG_RPC_DST_DEFAULT),
                       mch);
  }
  return true;
}
