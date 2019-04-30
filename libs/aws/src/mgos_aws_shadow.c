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

#include "common/cs_crc32.h"
#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "common/queue.h"
#include "frozen.h"
#include "mongoose.h"

#include "mgos_aws_shadow_internal.h"
#include "mgos_event.h"
#include "mgos_mongoose_internal.h"
#include "mgos_mqtt.h"
#include "mgos_shadow.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_utils.h"

#include "mgos_aws_shadow.h"

#define AWS_SHADOW_TOPIC_PREFIX "$aws/things/"
#define AWS_SHADOW_TOPIC_PREFIX_LEN (sizeof(AWS_SHADOW_TOPIC_PREFIX) - 1)
#define AWS_SHADOW_TOPIC_SHADOW "/shadow/"
#define AWS_SHADOW_TOPIC_SHADOW_LEN (sizeof(AWS_SHADOW_TOPIC_SHADOW) - 1)
#define TOKEN_LEN 8
#define TOKEN_BUF_SIZE (TOKEN_LEN + 1)

static uint64_t s_last_shadow_state_version;

enum mgos_aws_shadow_topic_id {
  MGOS_AWS_SHADOW_TOPIC_UNKNOWN = 0,
  MGOS_AWS_SHADOW_TOPIC_GET = 1,
  MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED = 2,
  MGOS_AWS_SHADOW_TOPIC_GET_REJECTED = 3,
  MGOS_AWS_SHADOW_TOPIC_UPDATE = 4,
  MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED = 5,
  MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED = 6,
  MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA = 7,
};

struct state_cb {
  mgos_aws_shadow_state_handler cb;
  void *userdata;
  SLIST_ENTRY(state_cb) link;
};

struct error_cb {
  mgos_aws_shadow_error_handler cb;
  void *userdata;
  SLIST_ENTRY(error_cb) link;
};

struct update_pending {
  struct mbuf data;
  STAILQ_ENTRY(update_pending) link;
};

struct aws_shadow_state {
  struct mgos_rlock_type *lock;
  struct mg_str thing_name;
  uint64_t current_version;
  unsigned int sub_id : 16;
  unsigned int connected : 1;
  unsigned int want_get : 1;
  unsigned int sent_get : 1;
  unsigned int have_get : 1;
  STAILQ_HEAD(update_entries, update_pending) update_entries;
  SLIST_HEAD(state_cb_entries, state_cb) state_cb_entries;
  SLIST_HEAD(error_cb_entries, error_cb) error_cb_entries;
};

static struct aws_shadow_state *s_shadow_state;

static char *get_aws_shadow_topic_name(const struct mg_str thing_name,
                                       enum mgos_aws_shadow_topic_id topic_id) {
  const char *s1 = NULL, *s2 = NULL;
  switch (topic_id) {
    case MGOS_AWS_SHADOW_TOPIC_UNKNOWN:
      return NULL;
    case MGOS_AWS_SHADOW_TOPIC_GET:
    case MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED:
    case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
      s1 = "get";
      break;
    case MGOS_AWS_SHADOW_TOPIC_UPDATE:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA:
      s1 = "update";
      break;
  }
  switch (topic_id) {
    case MGOS_AWS_SHADOW_TOPIC_UNKNOWN:
    case MGOS_AWS_SHADOW_TOPIC_GET:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE:
      s2 = "";
      break;
    case MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED:
      s2 = "/accepted";
      break;
    case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED:
      s2 = "/rejected";
      break;
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA:
      s2 = "/delta";
      break;
  }
  char *topic = NULL;
  mg_asprintf(&topic, 0, "%s%.*s%s%s%s", AWS_SHADOW_TOPIC_PREFIX,
              (int) thing_name.len, thing_name.p, AWS_SHADOW_TOPIC_SHADOW, s1,
              s2);
  mgos_expand_mac_address_placeholders(topic);
  return topic;
};

static enum mgos_aws_shadow_topic_id get_aws_shadow_topic_id(
    const struct mg_str topic, const struct mg_str thing_name) {
  struct mg_str s = topic;
  if (topic.len < AWS_SHADOW_TOPIC_PREFIX_LEN + thing_name.len +
                      AWS_SHADOW_TOPIC_SHADOW_LEN) {
    return MGOS_AWS_SHADOW_TOPIC_UNKNOWN;
  }
  if (mg_strncmp(
          s, mg_mk_str_n(AWS_SHADOW_TOPIC_PREFIX, AWS_SHADOW_TOPIC_PREFIX_LEN),
          AWS_SHADOW_TOPIC_PREFIX_LEN) != 0) {
    return MGOS_AWS_SHADOW_TOPIC_UNKNOWN;
  }
  s.p += AWS_SHADOW_TOPIC_PREFIX_LEN;
  s.len -= AWS_SHADOW_TOPIC_PREFIX_LEN;
  if (mg_strncmp(s, thing_name, thing_name.len) != 0) {
    return MGOS_AWS_SHADOW_TOPIC_UNKNOWN;
  }
  s.p += thing_name.len;
  s.len -= thing_name.len;
  if (mg_strncmp(
          s, mg_mk_str_n(AWS_SHADOW_TOPIC_SHADOW, AWS_SHADOW_TOPIC_SHADOW_LEN),
          AWS_SHADOW_TOPIC_SHADOW_LEN) != 0) {
    return MGOS_AWS_SHADOW_TOPIC_UNKNOWN;
  }
  s.p += AWS_SHADOW_TOPIC_SHADOW_LEN;
  s.len -= AWS_SHADOW_TOPIC_SHADOW_LEN;
  if (mg_strncmp(s, mg_mk_str_n("get", 3), 3) == 0) {
    s.p += 3;
    s.len -= 3;
    if (s.len == 0) return MGOS_AWS_SHADOW_TOPIC_GET;
    if (mg_vcmp(&s, "/accepted") == 0) {
      return MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED;
    } else if (mg_vcmp(&s, "/rejected") == 0) {
      return MGOS_AWS_SHADOW_TOPIC_GET_REJECTED;
    }
  } else if (mg_strncmp(s, mg_mk_str_n("update", 6), 6) == 0) {
    s.p += 6;
    s.len -= 6;
    if (s.len == 0) return MGOS_AWS_SHADOW_TOPIC_UPDATE;
    if (mg_vcmp(&s, "/accepted") == 0) {
      return MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED;
    } else if (mg_vcmp(&s, "/rejected") == 0) {
      return MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED;
    } else if (mg_vcmp(&s, "/delta") == 0) {
      return MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA;
    }
  }
  return MGOS_AWS_SHADOW_TOPIC_UNKNOWN;
}

static enum mgos_aws_shadow_event topic_id_to_aws_ev(
    enum mgos_aws_shadow_topic_id topic_id) {
  switch (topic_id) {
    case MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED:
      return MGOS_AWS_SHADOW_GET_ACCEPTED;
    case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
      return MGOS_AWS_SHADOW_GET_REJECTED;
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED:
      return MGOS_AWS_SHADOW_UPDATE_ACCEPTED;
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED:
      return MGOS_AWS_SHADOW_UPDATE_REJECTED;
    case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA:
      return MGOS_AWS_SHADOW_UPDATE_DELTA;
    case MGOS_AWS_SHADOW_TOPIC_GET:
    case MGOS_AWS_SHADOW_TOPIC_UPDATE:
    case MGOS_AWS_SHADOW_TOPIC_UNKNOWN:
      /* Can't happen */
      break;
  }
  return MGOS_AWS_SHADOW_UPDATE_DELTA; /* whatever */
}

static void calc_token(const struct aws_shadow_state *ss,
                       char token[TOKEN_BUF_SIZE]) {
  uint32_t crc = cs_crc32(0, ss->thing_name.p, ss->thing_name.len);
  sprintf(token, "%08x", (unsigned int) crc);
}

static bool is_our_token(const struct aws_shadow_state *ss,
                         const struct json_token token) {
  char exp_token[TOKEN_BUF_SIZE];
  calc_token(ss, exp_token);
  return (mg_strncmp(mg_mk_str_n(token.ptr, token.len),
                     mg_mk_str_n(exp_token, TOKEN_LEN), TOKEN_LEN) == 0);
}

static void mgos_aws_shadow_ev(struct mg_connection *nc, int ev, void *ev_data,
                               void *user_data) {
  struct aws_shadow_state *ss = (struct aws_shadow_state *) user_data;
  mgos_rlock(ss->lock);
  switch (ev) {
    case MG_EV_POLL: {
      if (!ss->connected) break;
      if (ss->want_get && !ss->sent_get) {
        LOG(LL_INFO, ("Requesting state, current version %llu",
                      (unsigned long long) ss->current_version));
        char *topic = get_aws_shadow_topic_name(ss->thing_name,
                                                MGOS_AWS_SHADOW_TOPIC_GET);
        struct mbuf buf;
        mbuf_init(&buf, 0);
        struct json_out out = JSON_OUT_MBUF(&buf);
        char token[TOKEN_BUF_SIZE];
        calc_token(ss, token);
        json_printf(&out, "{clientToken:\"%s\"}", token);
        mgos_mqtt_pub(topic, buf.buf, buf.len, 1 /* qos */, false /* retain */);
        ss->sent_get = true;
        mbuf_free(&buf);
        free(topic);
      }
      struct update_pending *up, *tmp;
      STAILQ_FOREACH_SAFE(up, &ss->update_entries, link, tmp) {
        char *topic = get_aws_shadow_topic_name(ss->thing_name,
                                                MGOS_AWS_SHADOW_TOPIC_UPDATE);
        LOG(LL_INFO,
            ("Update: %.*s", (int) MIN(200, up->data.len), up->data.buf));
        mgos_mqtt_pub(topic, up->data.buf, up->data.len, 1 /* qos */,
                      false /* retain */);
        STAILQ_REMOVE(&ss->update_entries, up, update_pending, link);
        mbuf_free(&up->data);
        free(up);
        free(topic);
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ss->connected) {
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_AWS};
        mgos_event_trigger(MGOS_EVENT_CLOUD_DISCONNECTED, &arg);
      }
      ss->connected = ss->sent_get = false;
      break;
    }
    case MG_EV_MQTT_CONNACK: {
      struct mg_mqtt_topic_expression topic_exprs[5] = {
          /* Hack: use QoS field to hold topic ID for now. */
          {.qos = MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED},
          {.qos = MGOS_AWS_SHADOW_TOPIC_GET_REJECTED},
          {.qos = MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED},
          {.qos = MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED},
          {.qos = MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA},
      };
      LOG(LL_DEBUG, ("Subscribing to topics"));
      for (int i = 0; i < 5; i++) {
        topic_exprs[i].topic = get_aws_shadow_topic_name(
            ss->thing_name, (enum mgos_aws_shadow_topic_id) topic_exprs[i].qos);
        topic_exprs[i].qos = 1;
        LOG(LL_DEBUG, ("  %s", topic_exprs[i].topic));
      }
      ss->sub_id = mgos_mqtt_get_packet_id();
      mg_mqtt_subscribe(nc, topic_exprs, 5 /* len */, ss->sub_id);
      for (int i = 0; i < 5; i++) {
        free((char *) topic_exprs[i].topic);
      }
      break;
    }
    case MG_EV_MQTT_SUBACK: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      if (msg->message_id == ss->sub_id) {
        LOG(LL_INFO, ("Subscribed"));
        ss->connected = ss->want_get = true;
        ss->sent_get = ss->have_get = false;
        const struct mg_str empty = mg_mk_str_n("", 0);
        struct state_cb *e;
        SLIST_FOREACH(e, &ss->state_cb_entries, link) {
          e->cb(e->userdata, MGOS_AWS_SHADOW_CONNECTED, 0, empty, empty, empty,
                empty);
        }
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_AWS};
        mgos_event_trigger(MGOS_EVENT_CLOUD_CONNECTED, &arg);
        mgos_event_trigger(MGOS_SHADOW_CONNECTED, NULL);
      }
      break;
    }
    case MG_EV_MQTT_PUBLISH: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      enum mgos_aws_shadow_topic_id topic_id =
          get_aws_shadow_topic_id(msg->topic, ss->thing_name);
      bool token_matches = false;
      struct json_token client_token = JSON_INVALID_TOKEN;
      switch (topic_id) {
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA:
        case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED: {
          /* Deltas and errors come without a token, assume they are for us. */
          token_matches = true;
          break;
        }
        default: {
          json_scanf(msg->payload.p, msg->payload.len, "{clientToken:%T}",
                     &client_token);
          token_matches = is_our_token(ss, client_token);
          break;
        }
      }
      LOG(LL_DEBUG,
          ("Topic %.*s (%d), id 0x%04x, token %.*s, payload:\r\n%.*s",
           (int) msg->topic.len, msg->topic.p, topic_id, msg->message_id,
           (int) client_token.len, client_token.ptr ? client_token.ptr : "",
           (int) msg->payload.len, msg->payload.p));
      if (!token_matches) {
        /*
         * This is not a response to one of our requests.
         * Still needs to be acked so that the broker doesn't lose patience with
         * us,
         * but we otherwise ignore it.
         */
        if (msg->qos > 0) mg_mqtt_puback(nc, msg->message_id);
        break;
      }
      if (topic_id == MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED ||
          topic_id == MGOS_AWS_SHADOW_TOPIC_GET_REJECTED) {
        ss->want_get = ss->sent_get = false;
      }
      switch (topic_id) {
        case MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA: {
          uint64_t version = 0;
          json_scanf(msg->payload.p, msg->payload.len, "{version:%llu}",
                     &version);
          LOG(LL_INFO, ("Version: %llu -> %llu (%d)",
                        (unsigned long long) ss->current_version,
                        (unsigned long long) version, topic_id));
          if (version < ss->current_version) {
            /* Thanks, not interested. */
            if (msg->qos > 0) mg_mqtt_puback(nc, msg->message_id);
            break;
          }
          if (topic_id == MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED) {
            ss->have_get = true;
          }
          {
            struct json_token st = JSON_INVALID_TOKEN;
            int ev = MGOS_SHADOW_CONNECTED + topic_id_to_aws_ev(topic_id);
            json_scanf(msg->payload.p, msg->payload.len, "{state:%T}", &st);
            struct mg_str state = {.p = st.ptr, .len = st.len};
            mgos_event_trigger(ev, &state);
          }
          LOG(LL_DEBUG,
              ("%d [%.*s]", ev, (int) msg->payload.len, msg->payload.p));

          if (SLIST_EMPTY(&ss->state_cb_entries)) {
            if (msg->qos > 0) mg_mqtt_puback(nc, msg->message_id);
            break;
          }
          struct json_token reported = JSON_INVALID_TOKEN;
          struct json_token desired = JSON_INVALID_TOKEN;
          struct json_token reported_md = JSON_INVALID_TOKEN;
          struct json_token desired_md = JSON_INVALID_TOKEN;
          /* Avoid NULL strings. */
          reported.ptr = desired.ptr = reported_md.ptr = desired_md.ptr = "";
          if (topic_id == MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA) {
            json_scanf(msg->payload.p, msg->payload.len,
                       "{state:%T,metadata:%T}", &desired, &desired_md);
          } else {
            json_scanf(msg->payload.p, msg->payload.len,
                       "{state:{reported:%T, desired:%T},"
                       "metadata:{reported:%T, desired:%T}}",
                       &reported, &desired, &reported_md, &desired_md);
          }
          mgos_runlock(ss->lock);
          struct state_cb *e;
          /* Only deliver deltas after a successful GET. */
          if (topic_id != MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA || ss->have_get) {
            SLIST_FOREACH(e, &ss->state_cb_entries, link) {
              e->cb(e->userdata, topic_id_to_aws_ev(topic_id), version,
                    mg_mk_str_n(reported.ptr, reported.len),
                    mg_mk_str_n(desired.ptr, desired.len),
                    mg_mk_str_n(reported_md.ptr, reported_md.len),
                    mg_mk_str_n(desired_md.ptr, desired_md.len));
            }
          }
          if (msg->qos > 0) mg_mqtt_puback(nc, msg->message_id);
          mgos_rlock(ss->lock);
          ss->current_version = version;
          break;
        }
        case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED: {
          struct mgos_shadow_error se = {.code = -1};
          char *message = NULL;
          if (msg->qos > 0) mg_mqtt_puback(nc, msg->message_id);
          json_scanf(msg->payload.p, msg->payload.len,
                     "{code: %d, message: %Q}", &se.code, &message);
          se.message.p = message ? message : "";
          se.message.len = strlen(message);
          mgos_event_trigger(
              MGOS_SHADOW_CONNECTED + topic_id_to_aws_ev(topic_id), &se);
          LOG(LL_ERROR, ("Error: %d %s", se.code, (message ? message : "")));
          mgos_runlock(ss->lock);
          struct error_cb *e;
          SLIST_FOREACH(e, &ss->error_cb_entries, link) {
            e->cb(e->userdata, topic_id_to_aws_ev(topic_id), se.code,
                  message ? message : "");
          }
          mgos_rlock(ss->lock);
          free(message);
          break;
        }
        /* We do not subscribe to GET and UPDATE */
        case MGOS_AWS_SHADOW_TOPIC_GET:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE:
        case MGOS_AWS_SHADOW_TOPIC_UNKNOWN:
          break;
      }
      break;
    }
  }
  mgos_runlock(ss->lock);
}

bool mgos_aws_shadow_set_state_handler(mgos_aws_shadow_state_handler state_cb,
                                       void *arg) {
  /* Deprecated as of 2017/12/14. */
  LOG(LL_WARN, ("mgos_aws_shadow is deprecated and will be removed soon, "
                "use the shadow lib with AWS backend instead "
                "(https://github.com/cesanta/mongoose-os/libs/shadow)."));
  if (s_shadow_state == NULL && !mgos_aws_shadow_init()) return false;
  mgos_rlock(s_shadow_state->lock);
  struct state_cb *e = calloc(1, sizeof(*e));
  e->cb = state_cb;
  e->userdata = arg;
  SLIST_INSERT_HEAD(&s_shadow_state->state_cb_entries, e, link);
  mgos_runlock(s_shadow_state->lock);
  return true;
}

bool mgos_aws_shadow_set_error_handler(mgos_aws_shadow_error_handler error_cb,
                                       void *arg) {
  if (s_shadow_state == NULL && !mgos_aws_shadow_init()) return false;
  mgos_rlock(s_shadow_state->lock);
  struct error_cb *e = calloc(1, sizeof(*e));
  e->cb = error_cb;
  e->userdata = arg;
  SLIST_INSERT_HEAD(&s_shadow_state->error_cb_entries, e, link);
  mgos_runlock(s_shadow_state->lock);
  return true;
}

bool mgos_aws_shadow_get(void) {
  if (s_shadow_state == NULL && !mgos_aws_shadow_init()) return false;
  mgos_rlock(s_shadow_state->lock);
  s_shadow_state->want_get = true;
  mgos_runlock(s_shadow_state->lock);
  mongoose_schedule_poll(false /* from_isr */);
  return true;
}

bool mgos_aws_shadow_updatevf(uint64_t version, const char *state_jsonf,
                              va_list ap) {
  bool res = false;
  struct update_pending *up = NULL;
  if (s_shadow_state == NULL && !mgos_aws_shadow_init()) return false;
  up = (struct update_pending *) calloc(1, sizeof(*up));
  if (up == NULL) return false;
  mbuf_init(&up->data, 50);
  char token[TOKEN_BUF_SIZE];
  calc_token(s_shadow_state, token);
  struct json_out out = JSON_OUT_MBUF(&up->data);
  json_printf(&out, "{state: {reported: ");
  json_vprintf(&out, state_jsonf, ap);
  json_printf(&out, "}");
  if (version > 0) {
    json_printf(&out, ", version: %llu", version);
  }
  json_printf(&out, ", clientToken: \"%s\"}", token);
  mgos_rlock(s_shadow_state->lock);
  STAILQ_INSERT_TAIL(&s_shadow_state->update_entries, up, link);
  mgos_runlock(s_shadow_state->lock);
  mongoose_schedule_poll(false /* from_isr */);
  res = true;
  return res;
}

bool mgos_aws_shadow_updatef(uint64_t version, const char *state_jsonf, ...) {
  if (s_shadow_state == NULL && !mgos_aws_shadow_init()) return false;
  va_list ap;
  va_start(ap, state_jsonf);
  bool res = mgos_aws_shadow_updatevf(version, state_jsonf, ap);
  va_end(ap);
  return res;
}

bool mgos_aws_shadow_update_simple(double version, const char *state_json) {
  return mgos_aws_shadow_updatef(version, "%s", state_json);
}

const char *mgos_aws_shadow_event_name(enum mgos_aws_shadow_event ev) {
  switch (ev) {
    case MGOS_AWS_SHADOW_CONNECTED:
      return "CONNECTED";
    case MGOS_AWS_SHADOW_GET_ACCEPTED:
      return "GET_ACCEPTED";
    case MGOS_AWS_SHADOW_GET_REJECTED:
      return "GET_REJECTED";
    case MGOS_AWS_SHADOW_UPDATE_ACCEPTED:
      return "UPDATE_ACCEPTED";
    case MGOS_AWS_SHADOW_UPDATE_REJECTED:
      return "UPDATE_REJECTED";
    case MGOS_AWS_SHADOW_UPDATE_DELTA:
      return "UPDATE_DELTA";
  }
  return "";
}

bool mgos_aws_is_connected(void) {
  return (s_shadow_state != NULL && s_shadow_state->connected);
}

static void update_cb(int ev, void *ev_data, void *userdata) {
  struct mgos_shadow_update_data *data = ev_data;
  mgos_aws_shadow_updatevf(data->version, data->json_fmt, data->ap);
  (void) userdata;
  (void) ev;
}

bool mgos_aws_shadow_init(void) {
  const char *impl = mgos_sys_config_get_shadow_lib();
  if (!mgos_sys_config_get_shadow_enable()) return true;
  if (impl != NULL && strcmp(impl, "aws") != 0) {
    LOG(LL_DEBUG, ("shadow.lib=%s, not initialising AWS shadow", impl));
    return false;
  }
  if (!mgos_sys_config_get_mqtt_enable()) {
    LOG(LL_DEBUG, ("AWS Device Shadow requires MQTT"));
    return false;
  }
  const char *thing_name = NULL;
  if ((thing_name = mgos_sys_config_get_aws_thing_name()) == NULL) {
    LOG(LL_ERROR, ("AWS Device Shadow requires thing_name or device.id"));
    return false;
  }
  const char *mqtt_server = mgos_sys_config_get_mqtt_server();
  if (mqtt_server == NULL ||
      (impl == NULL && strstr(mqtt_server, "amazonaws.com") == NULL)) {
    LOG(LL_ERROR, ("MQTT is not configured for AWS, not initialising shadow"));
    return false;
  }

  struct aws_shadow_state *ss =
      (struct aws_shadow_state *) calloc(1, sizeof(*ss));
  ss->lock = mgos_rlock_create();
  ss->thing_name = mg_mk_str(thing_name);
  STAILQ_INIT(&ss->update_entries);
  SLIST_INIT(&ss->state_cb_entries);
  SLIST_INIT(&ss->error_cb_entries);
  mgos_mqtt_add_global_handler(mgos_aws_shadow_ev, ss);
  s_shadow_state = ss;
  char token[TOKEN_BUF_SIZE];
  calc_token(ss, token);
  mgos_event_add_handler(MGOS_SHADOW_GET,
                         (mgos_event_handler_t) mgos_aws_shadow_get, NULL);
  mgos_event_add_handler(MGOS_SHADOW_UPDATE, update_cb, NULL);
  LOG(LL_INFO, ("Device shadow name: %.*s (token %s)", (int) ss->thing_name.len,
                ss->thing_name.p, token));
  return true;
}

/*
 * Data for the FFI-able wrapper
 */
struct mgos_aws_shadow_cb_simple_data {
  /* FFI-able callback and its user_data */
  mgos_aws_shadow_state_handler_simple cb_simple;
  void *cb_arg;
};

static void state_cb_oplya(void *arg, enum mgos_aws_shadow_event ev,
                           uint64_t version, const struct mg_str reported,
                           const struct mg_str desired,
                           const struct mg_str reported_md,
                           const struct mg_str desired_md) {
  struct mgos_aws_shadow_cb_simple_data *oplya_arg =
      (struct mgos_aws_shadow_cb_simple_data *) arg;

  /*
   * FFI expects strings to be NUL-terminated, so we have to reallocate
   * `mg_str`s.
   *
   * TODO(dfrank): implement a way to ffi strings via pointer + length
   */

  struct mg_str reported2 = mg_strdup_nul(reported);
  struct mg_str desired2 = mg_strdup_nul(desired);
  struct mg_str reported_md2 = mg_strdup_nul(reported_md);
  struct mg_str desired_md2 = mg_strdup_nul(desired_md);

  oplya_arg->cb_simple(oplya_arg->cb_arg, ev, reported2.p, desired2.p,
                       reported_md2.p, desired_md2.p);

  free((void *) reported2.p);
  free((void *) desired2.p);
  free((void *) reported_md2.p);
  free((void *) desired_md2.p);

  s_last_shadow_state_version = version;
}

bool mgos_aws_shadow_set_state_handler_simple(
    mgos_aws_shadow_state_handler_simple state_cb_simple, void *arg) {
  /* NOTE: it won't be freed */
  struct mgos_aws_shadow_cb_simple_data *oplya_arg =
      calloc(1, sizeof(*oplya_arg));
  oplya_arg->cb_simple = state_cb_simple;
  oplya_arg->cb_arg = arg;

  return mgos_aws_shadow_set_state_handler(state_cb_oplya, oplya_arg);
}

double mgos_aws_shadow_get_last_state_version(void) {
  return (double) s_last_shadow_state_version;
}
