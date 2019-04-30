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

#include "mgos_azure_internal.h"

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "common/queue.h"

#include "mgos_mongoose_internal.h"
#include "mgos_mqtt.h"
#include "mgos_shadow.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_utils.h"

#define AZURE_TWIN_RES_TOPIC "$iothub/twin/res/"
#define AZURE_TWIN_DELTA_TOPIC "$iothub/twin/PATCH/properties/desired/"
#define AZURE_TWIN_GET_TOPIC "$iothub/twin/GET/?$rid=get%u"
#define AZURE_TWIN_UPDATE_TOPIC \
  "$iothub/twin/PATCH/properties/reported/?rid=upd%u"

struct azure_shadow_update {
  struct mbuf data;
  STAILQ_ENTRY(azure_shadow_update) next;
};

struct azure_shadow_state {
  struct mgos_azure_ctx *ctx;
  struct mgos_rlock_type *lock;
  uint64_t current_version;
  unsigned int sub_id : 16;
  unsigned int connected : 1;
  unsigned int want_get : 1;
  unsigned int sent_get : 1;
  unsigned int have_get : 1;
  STAILQ_HEAD(updates, azure_shadow_update) updates;
};

static void mgos_azure_shadow_mqtt_ev(struct mg_connection *nc, int ev,
                                      void *ev_data, void *user_data) {
  struct azure_shadow_state *ss = (struct azure_shadow_state *) user_data;
  mgos_rlock(ss->lock);
  switch (ev) {
    case MG_EV_MQTT_CONNACK: {
      struct mg_mqtt_topic_expression topics[2] = {
          {.topic = AZURE_TWIN_RES_TOPIC "#", .qos = 1},
          {.topic = AZURE_TWIN_DELTA_TOPIC "#", .qos = 1},
      };
      ss->sub_id = mgos_mqtt_get_packet_id();
      mg_mqtt_subscribe(nc, topics, ARRAY_SIZE(topics), ss->sub_id);
      break;
    }
    case MG_EV_MQTT_SUBACK: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      if (msg->message_id != ss->sub_id || ss->connected) break;
      ss->connected = ss->want_get = true;
      ss->sent_get = ss->have_get = false;
      ss->ctx->have_acks++;
      mgos_azure_trigger_connected(ss->ctx);
      mgos_event_trigger(MGOS_SHADOW_CONNECTED, NULL);
    }
    /* fall-through */
    case MG_EV_POLL: {
      if (!ss->connected) break;
      if (ss->want_get && !ss->sent_get) {
        char *topic = NULL;
        mg_asprintf(&topic, 0, AZURE_TWIN_GET_TOPIC, mgos_mqtt_get_packet_id());
        if (topic == NULL) break;
        mgos_mqtt_pub(topic, NULL, 0, 0 /* qos */, false /* retain */);
        ss->sent_get = true;
        free(topic);
        topic = NULL;
      }
      struct azure_shadow_update *up, *upt;
      STAILQ_FOREACH_SAFE(up, &ss->updates, next, upt) {
        char *topic = NULL;
        LOG(LL_INFO,
            ("Update: %.*s", (int) MIN(200, up->data.len), up->data.buf));
        mg_asprintf(&topic, 0, AZURE_TWIN_UPDATE_TOPIC,
                    mgos_mqtt_get_packet_id());
        if (topic == NULL) break;
        mgos_mqtt_pub(topic, up->data.buf, up->data.len, 1 /* qos */,
                      false /* retain */);
        STAILQ_REMOVE(&ss->updates, up, azure_shadow_update, next);
        mbuf_free(&up->data);
        free(up);
        free(topic);
        topic = NULL;
      }
      break;
    }
    case MG_EV_MQTT_PUBLISH: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      struct mgos_shadow_error err = {.code = 0};
      LOG(LL_DEBUG, ("'%.*s'", (int) msg->topic.len, msg->topic.p));
      LOG(LL_DEBUG, ("'%.*s'", (int) msg->payload.len, msg->payload.p));
      int ev = 0;
      void *arg = NULL;
      if (mg_strstr(msg->topic, mg_mk_str(AZURE_TWIN_RES_TOPIC)) != NULL) {
        if (mg_strstr(msg->topic, mg_mk_str("rid=get")) != NULL) {
          ss->want_get = ss->sent_get = false;
          const char *res = mg_strstr(msg->topic, mg_mk_str("/res/"));
          if (res != NULL) {
            err.code = strtol(res + 5, NULL, 10);
            if (err.code == 200) {
              ev = MGOS_SHADOW_GET_ACCEPTED;
              arg = &msg->payload;
              ss->have_get = true;
            } else {
              ev = MGOS_SHADOW_GET_REJECTED;
              err.message = msg->payload;
            }
          }
        } else {
          /* Azure doesn't send responses to updates, actually. */
        }
      } else if (mg_strstr(msg->topic, mg_mk_str(AZURE_TWIN_DELTA_TOPIC)) !=
                 NULL) {
        if (ss->have_get) {
          /* Only deliver deltas after a successful GET. */
          ev = MGOS_SHADOW_UPDATE_DELTA;
          arg = &msg->payload;
        }
      }
      if (ev != 0) mgos_event_trigger(ev, arg);
      break;
    }
    case MG_EV_CLOSE:
      ss->connected = ss->sent_get = false;
      break;
  }
  mgos_runlock(ss->lock);
}

static void azure_shadow_get_req_ev(int ev, void *ev_data, void *user_data) {
  struct azure_shadow_state *ss = (struct azure_shadow_state *) user_data;
  mgos_rlock(ss->lock);
  ss->want_get = true;
  mgos_runlock(ss->lock);
  mongoose_schedule_poll(false /* from_isr */);
  (void) ev_data;
  (void) ev;
}

static void azure_shadow_update_req_ev(int ev, void *ev_data, void *user_data) {
  struct mgos_shadow_update_data *data =
      (struct mgos_shadow_update_data *) ev_data;
  struct azure_shadow_state *ss = (struct azure_shadow_state *) user_data;
  struct azure_shadow_update *up =
      (struct azure_shadow_update *) calloc(1, sizeof(*up));
  mbuf_init(&up->data, 50);
  struct json_out out = JSON_OUT_MBUF(&up->data);
  json_vprintf(&out, data->json_fmt, data->ap);
  mgos_rlock(ss->lock);
  STAILQ_INSERT_TAIL(&ss->updates, up, next);
  mgos_runlock(ss->lock);
  mongoose_schedule_poll(false /* from_isr */);
  (void) ev;
}

bool mgos_azure_shadow_init(struct mgos_azure_ctx *ctx) {
  const char *impl = mgos_sys_config_get_shadow_lib();
  if (!mgos_sys_config_get_shadow_enable()) return true;
  if (impl != NULL && strcmp(impl, "azure") != 0) {
    LOG(LL_DEBUG, ("shadow.lib=%s, not initialising Azure shadow", impl));
    return false;
  }
  if (!mgos_sys_config_get_azure_enable()) {
    LOG(LL_ERROR, ("AWS Device Shadow requires azure.enable"));
    return false;
  }

  struct azure_shadow_state *ss =
      (struct azure_shadow_state *) calloc(1, sizeof(*ss));
  ctx->want_acks++;
  ss->ctx = ctx;
  ss->lock = mgos_rlock_create();
  STAILQ_INIT(&ss->updates);
  mgos_event_add_handler(MGOS_SHADOW_GET,
                         (mgos_event_handler_t) azure_shadow_get_req_ev, ss);
  mgos_event_add_handler(MGOS_SHADOW_UPDATE, azure_shadow_update_req_ev, ss);
  mgos_mqtt_add_global_handler(mgos_azure_shadow_mqtt_ev, ss);
  struct mg_str did = mgos_azure_get_device_id();
  LOG(LL_INFO,
      ("Azure Device Twin enabled, device %.*s", (int) did.len, did.p));
  return true;
}
