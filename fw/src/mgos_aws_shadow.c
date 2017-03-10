/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_aws_shadow.h"

#include <stdlib.h>

#include "common/cs_crc32.h"
#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mg_str.h"
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mqtt.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_utils.h"

#define AWS_SHADOW_SUB_ID 0x1234
#define AWS_SHADOW_TOPIC_PREFIX "$aws/things/"
#define AWS_SHADOW_TOPIC_PREFIX_LEN (sizeof(AWS_SHADOW_TOPIC_PREFIX) - 1)
#define AWS_SHADOW_TOPIC_SHADOW "/shadow/"
#define AWS_SHADOW_TOPIC_SHADOW_LEN (sizeof(AWS_SHADOW_TOPIC_SHADOW) - 1)
#define TOKEN_LEN 8
#define TOKEN_BUF_SIZE (TOKEN_LEN + 1)

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

struct aws_shadow_state {
  struct mg_str thing_name;
  uint64_t current_version;
  unsigned int online : 1;
  unsigned int want_get : 1;
  unsigned int sent_get : 1;
  struct mbuf update;
  mgos_aws_shadow_state_handler state_cb;
  mgos_aws_shadow_error_handler error_cb;
  void *state_cb_arg;
  void *error_cb_arg;
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
      return MGOS_AWS_SHADOW_UPDATE_REJECTED;
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

static void mgos_aws_shadow_ev(struct mg_connection *nc, int ev,
                               void *ev_data) {
  struct aws_shadow_state *ss = (struct aws_shadow_state *) nc->user_data;
  mgos_lock();
  switch (ev) {
    case MG_EV_POLL: {
      if (!ss->online) break;
      if (ss->want_get && !ss->sent_get) {
        LOG(LL_INFO,
            ("Requesting state, current version %llu", ss->current_version));
        char *topic = get_aws_shadow_topic_name(ss->thing_name,
                                                MGOS_AWS_SHADOW_TOPIC_GET);
        struct mbuf buf;
        mbuf_init(&buf, 0);
        struct json_out out = JSON_OUT_MBUF(&buf);
        char token[TOKEN_BUF_SIZE];
        calc_token(ss, token);
        json_printf(&out, "{clientToken:\"%s\"}", token);
        mg_mqtt_publish(nc, topic, 0 /* message_id */, MG_MQTT_QOS(1), buf.buf,
                        buf.len);
        ss->sent_get = true;
        mbuf_free(&buf);
        free(topic);
      }
      if (ss->update.len > 0) {
        char *topic = get_aws_shadow_topic_name(ss->thing_name,
                                                MGOS_AWS_SHADOW_TOPIC_UPDATE);
        LOG(LL_INFO,
            ("Update: %.*s", (int) MIN(200, ss->update.len), ss->update.buf));
        mg_mqtt_publish(nc, topic, 0 /* message_id */, MG_MQTT_QOS(1),
                        ss->update.buf, ss->update.len);
        mbuf_remove(&ss->update, ss->update.len);
        mbuf_trim(&ss->update);
        free(topic);
      }
      break;
    }
    case MG_EV_CLOSE: {
      ss->online = ss->sent_get = false;
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
      mg_mqtt_subscribe(nc, topic_exprs, 5 /* len */,
                        AWS_SHADOW_SUB_ID /* sub_id */);
      for (int i = 0; i < 5; i++) {
        free((char *) topic_exprs[i].topic);
      }
      break;
    }
    case MG_EV_MQTT_SUBACK: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      if (msg->message_id == AWS_SHADOW_SUB_ID) {
        LOG(LL_INFO, ("Subscribed"));
        ss->online = true;
        ss->want_get = get_cfg()->aws.shadow.get_on_connect;
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
        mg_mqtt_puback(nc, msg->message_id);
        break;
      }
      if (topic_id == MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED ||
          topic_id == MGOS_AWS_SHADOW_TOPIC_GET_REJECTED) {
        ss->want_get = false;
      }
      switch (topic_id) {
        case MGOS_AWS_SHADOW_TOPIC_GET_ACCEPTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_ACCEPTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA: {
          uint64_t version = 0;
          json_scanf(msg->payload.p, msg->payload.len, "{version:%llu}",
                     &version);
          LOG(LL_INFO, ("Version: %llu -> %llu (%d)", ss->current_version,
                        version, topic_id));
          if (version < ss->current_version) {
            /* Thanks, not interested. */
            mg_mqtt_puback(nc, msg->message_id);
            break;
          }
          if (ss->state_cb == NULL) {
            LOG(LL_WARN, ("No state handler, message ignored."));
            mg_mqtt_puback(nc, msg->message_id);
            break;
          }
          struct json_token reported = JSON_INVALID_TOKEN;
          struct json_token desired = JSON_INVALID_TOKEN;
          reported.ptr = desired.ptr = ""; /* Avoid NULL strings. */
          if (topic_id == MGOS_AWS_SHADOW_TOPIC_UPDATE_DELTA) {
            json_scanf(msg->payload.p, msg->payload.len, "{state:%T}",
                       &desired);
          } else {
            json_scanf(msg->payload.p, msg->payload.len,
                       "{state:{reported:%T, desired:%T}}", &reported,
                       &desired);
          }
          if (ss->state_cb(ss->state_cb_arg, topic_id_to_aws_ev(topic_id),
                           version, mg_mk_str_n(reported.ptr, reported.len),
                           mg_mk_str_n(desired.ptr, desired.len))) {
            mgos_unlock();
            mg_mqtt_puback(nc, msg->message_id);
            mgos_lock();
            ss->current_version = version;
          }
          break;
        }
        case MGOS_AWS_SHADOW_TOPIC_GET_REJECTED:
        case MGOS_AWS_SHADOW_TOPIC_UPDATE_REJECTED: {
          mg_mqtt_puback(nc, msg->message_id);
          int code = -1;
          char *message = NULL;
          json_scanf(msg->payload.p, msg->payload.len,
                     "{code: %d, message: %Q}", &code, &message);
          LOG(LL_ERROR, ("Error: %d %s", code, (message ? message : "")));
          if (ss->error_cb != NULL) {
            mgos_unlock();
            ss->error_cb(ss->error_cb_arg, topic_id_to_aws_ev(topic_id), code,
                         message ? message : "");
            mgos_lock();
          }
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
  mgos_unlock();
}

void mgos_aws_shadow_set_state_handler(mgos_aws_shadow_state_handler state_cb,
                                       void *arg) {
  if (s_shadow_state == NULL) return;
  mgos_lock();
  s_shadow_state->state_cb = state_cb;
  s_shadow_state->state_cb_arg = arg;
  mgos_unlock();
}

void mgos_aws_shadow_set_error_handler(mgos_aws_shadow_error_handler error_cb,
                                       void *arg) {
  if (s_shadow_state == NULL) return;
  mgos_lock();
  s_shadow_state->error_cb = error_cb;
  s_shadow_state->error_cb_arg = arg;
  mgos_unlock();
}

bool mgos_aws_shadow_get(void) {
  if (s_shadow_state == NULL) return false;
  mgos_lock();
  s_shadow_state->want_get = true;
  mgos_unlock();
  mongoose_schedule_poll(false /* from_isr */);
  return true;
}

bool mgos_aws_shadow_updatef(uint64_t version, const char *state_jsonf, ...) {
  if (s_shadow_state == NULL) return false;
  mgos_lock();
  if (s_shadow_state->update.len > 0) {
    mbuf_free(&s_shadow_state->update);
    mbuf_init(&s_shadow_state->update, 50);
  }
  char token[TOKEN_BUF_SIZE];
  calc_token(s_shadow_state, token);
  struct json_out out = JSON_OUT_MBUF(&s_shadow_state->update);
  json_printf(&out, "{state: ");
  va_list ap;
  va_start(ap, state_jsonf);
  json_vprintf(&out, state_jsonf, ap);
  va_end(ap);
  if (version > 0) {
    json_printf(&out, ", version: %llu", version);
  }
  json_printf(&out, ", clientToken: \"%s\"}", token);
  mgos_unlock();
  mongoose_schedule_poll(false /* from_isr */);
  return true;
}

bool mgos_aws_shadow_update_simple(double version, const char *state_json) {
  return mgos_aws_shadow_updatef(version, "%s", state_json);
}

enum mgos_init_result mgos_aws_shadow_init(void) {
  struct sys_config *cfg = get_cfg();
  if (!cfg->aws.shadow.enable) return MGOS_INIT_OK;
  if (!cfg->mqtt.enable) {
    LOG(LL_ERROR, ("AWS Device Shadow requires MQTT"));
    return MGOS_INIT_AWS_SHADOW_INIT_FAILED;
  }
  if (cfg->device.id == NULL) {
    LOG(LL_ERROR, ("AWS Device Shadow requires device.id"));
    return MGOS_INIT_AWS_SHADOW_INIT_FAILED;
  }
  struct aws_shadow_state *ss =
      (struct aws_shadow_state *) calloc(1, sizeof(*ss));
  ss->thing_name = mg_mk_str(get_cfg()->device.id);
  mgos_mqtt_add_global_handler(mgos_aws_shadow_ev, ss);
  s_shadow_state = ss;
  char token[TOKEN_BUF_SIZE];
  calc_token(ss, token);
  LOG(LL_INFO, ("Device shadow name: %.*s (token %s)", (int) ss->thing_name.len,
                ss->thing_name.p, token));
  return MGOS_INIT_OK;
}

/*
 * Data for the FFI-able wrapper
 */
struct mgos_aws_shadow_cb_simple_data {
  /* FFI-able callback and its userdata */
  mgos_aws_shadow_state_handler_simple cb_simple;
  void *cb_arg;
};

static bool state_cb_oplya(void *arg, enum mgos_aws_shadow_event ev,
                           uint64_t version, const struct mg_str reported,
                           const struct mg_str desired) {
  bool ret = false;
  struct mgos_aws_shadow_cb_simple_data *oplya_arg =
      (struct mgos_aws_shadow_cb_simple_data *) arg;

  /*
   * FFI expects strings to be null-terminated, so we have to reallocate
   * `mg_str`s.
   *
   * TODO(dfrank): implement a way to ffi strings via pointer + length
   */

  char *reported2 = calloc(1, reported.len + 1 /* null-terminate */);
  char *desired2 = calloc(1, desired.len + 1 /* null-terminate */);

  memcpy(reported2, reported.p, reported.len);
  memcpy(desired2, desired.p, desired.len);

  ret = oplya_arg->cb_simple(oplya_arg->cb_arg, ev, reported2, desired2);

  free(reported2);
  free(desired2);

  (void) version;

  return ret;
}

void mgos_aws_shadow_set_state_handler_simple(
    mgos_aws_shadow_state_handler_simple state_cb_simple, void *arg) {
  /* NOTE: it won't be freed */
  struct mgos_aws_shadow_cb_simple_data *oplya_arg =
      calloc(1, sizeof(*oplya_arg));
  oplya_arg->cb_simple = state_cb_simple;
  oplya_arg->cb_arg = arg;

  mgos_aws_shadow_set_state_handler(state_cb_oplya, oplya_arg);
}
