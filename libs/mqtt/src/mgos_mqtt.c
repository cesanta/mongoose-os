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

#include "mgos_mqtt.h"

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "common/platform.h"
#include "common/queue.h"
#include "frozen.h"
#include "mgos_debug.h"
#include "mgos_event.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_utils.h"

#ifndef MGOS_MQTT_LOG_PUSHBACK_THRESHOLD
#define MGOS_MQTT_LOG_PUSHBACK_THRESHOLD 2048
#endif

#ifndef MGOS_MQTT_SUBSCRIBE_QOS
#define MGOS_MQTT_SUBSCRIBE_QOS 1
#endif

struct topic_handler {
  struct mg_str topic;
  mg_event_handler_t handler;
  void *user_data;
  uint8_t qos;
  uint16_t sub_id;
  SLIST_ENTRY(topic_handler) entries;
};

struct global_handler {
  mg_event_handler_t handler;
  void *user_data;
  SLIST_ENTRY(global_handler) entries;
};

static int s_reconnect_timeout_ms = 0;
static mgos_timer_id s_reconnect_timer_id = MGOS_INVALID_TIMER_ID;
static struct mg_connection *s_conn = NULL;
static bool s_connected = false;
static mgos_mqtt_connect_fn_t s_connect_fn = NULL;
static void *s_connect_fn_arg = NULL;
static const struct mgos_config_mqtt *s_cfg = NULL;
static int s_max_qos = -1;

SLIST_HEAD(topic_handlers, topic_handler) s_topic_handlers;
SLIST_HEAD(global_handlers, global_handler) s_global_handlers;

static void mqtt_global_reconnect(void);

void mgos_mqtt_set_max_qos(int qos) {
  if (s_cfg == NULL || s_cfg->max_qos == qos) return;
  LOG(LL_INFO, ("Setting max MQTT QOS to %d", qos));
  s_max_qos = qos;
}

static int adjust_qos(int qos) {
  int max_qos = s_max_qos;
  if (max_qos < 0 && s_cfg != NULL) max_qos = s_cfg->max_qos;
  if (max_qos < 0) return qos;
  return MIN(qos, max_qos);
}

uint16_t mgos_mqtt_get_packet_id(void) {
  static uint16_t s_packet_id = 0;
  s_packet_id++;
  if (s_packet_id == 0) s_packet_id++;
  return s_packet_id;
}

static bool call_topic_handler(struct mg_connection *nc, int ev, void *ev_data,
                               void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
  struct topic_handler *th;
  SLIST_FOREACH(th, &s_topic_handlers, entries) {
    if ((ev == MG_EV_MQTT_SUBACK && th->sub_id == msg->message_id) ||
        mg_mqtt_match_topic_expression(th->topic, msg->topic)) {
      if (ev == MG_EV_MQTT_PUBLISH && msg->qos > 0) {
        mg_mqtt_puback(nc, msg->message_id);
      }
      th->handler(nc, ev, ev_data, th->user_data);
      return true;
    }
  }
  (void) user_data;
  return false;
}

static void call_global_handlers(struct mg_connection *nc, int ev,
                                 void *ev_data, void *user_data) {
  struct global_handler *gh;
  SLIST_FOREACH(gh, &s_global_handlers, entries) {
    gh->handler(nc, ev, ev_data, gh->user_data);
  }
  (void) user_data;
}

static void do_subscribe(struct topic_handler *th) {
  struct mg_mqtt_topic_expression te = {.topic = th->topic.p,
                                        .qos = adjust_qos(th->qos)};
  th->sub_id = mgos_mqtt_get_packet_id();
  mg_mqtt_subscribe(mgos_mqtt_get_global_conn(), &te, 1, th->sub_id);
  LOG(LL_INFO, ("Subscribing to '%s' (QoS %d)", te.topic, te.qos));
}

static void mgos_mqtt_ev(struct mg_connection *nc, int ev, void *ev_data,
                         void *user_data) {
  if (nc != s_conn) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return;
  }
  if (ev > MG_MQTT_EVENT_BASE) {
    LOG(LL_DEBUG, ("MQTT event: %d", ev));
  }
  switch (ev) {
    case MG_EV_CONNECT: {
      int status = *((int *) ev_data);
      LOG(LL_INFO,
          ("MQTT TCP connect %s (%d)", (status == 0 ? "ok" : "error"), status));
      if (status != 0) break;
      struct mg_send_mqtt_handshake_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.user_name = s_cfg->user;
      opts.password = s_cfg->pass;
      if (s_cfg->clean_session) {
        opts.flags |= MG_MQTT_CLEAN_SESSION;
      }
      opts.keep_alive = s_cfg->keep_alive;
      opts.will_topic = s_cfg->will_topic;
      opts.will_message = s_cfg->will_message;
      const char *client_id =
          (s_cfg->client_id != NULL ? s_cfg->client_id
                                    : mgos_sys_config_get_device_id());
      if (s_connect_fn != NULL) {
        s_connect_fn(nc, client_id, &opts, s_connect_fn_arg);
      } else {
        mg_send_mqtt_handshake_opt(nc, client_id, opts);
      }
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("MQTT Disconnect"));
      s_conn = NULL;
      s_connected = false;
      if (s_cfg->cloud_events) {
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_MQTT};
        mgos_event_trigger(MGOS_EVENT_CLOUD_DISCONNECTED, &arg);
      }
      call_global_handlers(nc, ev, NULL, user_data);
      mqtt_global_reconnect();
      break;
    }
    case MG_EV_POLL: {
      call_global_handlers(nc, ev, NULL, user_data);
      break;
    }
    case MG_EV_MQTT_CONNACK: {
      struct topic_handler *th;
      int code = ((struct mg_mqtt_message *) ev_data)->connack_ret_code;
      LOG((code == 0 ? LL_INFO : LL_ERROR), ("MQTT CONNACK %d", code));
      if (code == 0) {
        s_connected = true;
        s_reconnect_timeout_ms = 0;
        if (s_cfg->cloud_events) {
          struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_MQTT};
          mgos_event_trigger(MGOS_EVENT_CLOUD_CONNECTED, &arg);
        }
        call_global_handlers(nc, ev, ev_data, user_data);
        SLIST_FOREACH(th, &s_topic_handlers, entries) {
          do_subscribe(th);
        }
      } else {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    /* Delegate almost all MQTT events to the user's handler */
    case MG_EV_MQTT_SUBACK:
    case MG_EV_MQTT_PUBLISH:
      if (call_topic_handler(nc, ev, ev_data, user_data)) break;
    /* fall through */
    case MG_EV_MQTT_PUBACK:
    case MG_EV_MQTT_CONNECT:
    case MG_EV_MQTT_PUBREL:
    case MG_EV_MQTT_PUBCOMP:
    case MG_EV_MQTT_SUBSCRIBE:
    case MG_EV_MQTT_UNSUBSCRIBE:
    case MG_EV_MQTT_UNSUBACK:
    case MG_EV_MQTT_PINGREQ:
    case MG_EV_MQTT_DISCONNECT:
      call_global_handlers(nc, ev, ev_data, user_data);
      break;
    case MG_EV_MQTT_PUBREC: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      mg_mqtt_pubrel(nc, msg->message_id);
      call_global_handlers(nc, ev, ev_data, user_data);
      break;
    }
  }
}

void mgos_mqtt_global_subscribe(const struct mg_str topic,
                                mg_event_handler_t handler, void *ud) {
  struct topic_handler *th = (struct topic_handler *) calloc(1, sizeof(*th));
  th->topic.p = (char *) calloc(topic.len + 1 /* + NUL */, 1);
  memcpy((char *) th->topic.p, topic.p, topic.len);
  th->topic.len = topic.len;
  th->handler = handler;
  th->user_data = ud;
  th->qos = adjust_qos(MGOS_MQTT_SUBSCRIBE_QOS);
  SLIST_INSERT_HEAD(&s_topic_handlers, th, entries);
  if (s_connected) do_subscribe(th);
}

void mgos_mqtt_add_global_handler(mg_event_handler_t handler, void *ud) {
  struct global_handler *gh = (struct global_handler *) calloc(1, sizeof(*gh));
  gh->handler = handler;
  gh->user_data = ud;
  SLIST_INSERT_HEAD(&s_global_handlers, gh, entries);
}

void mgos_mqtt_set_connect_fn(mgos_mqtt_connect_fn_t fn, void *fn_arg) {
  s_connect_fn = fn;
  s_connect_fn_arg = fn_arg;
}

static void mgos_mqtt_net_ev(int ev, void *evd, void *arg) {
  if (ev != MGOS_NET_EV_IP_ACQUIRED) return;

  mgos_mqtt_global_connect();
  (void) evd;
  (void) arg;
}

static void s_debug_write_cb(int ev, void *ev_data, void *userdata) {
  struct mgos_debug_hook_arg *arg = (struct mgos_debug_hook_arg *) ev_data;
  const char *topic =
      (arg->fd == 1
           ? mgos_sys_config_get_debug_stdout_topic()
           : arg->fd == 2 ? mgos_sys_config_get_debug_stderr_topic() : NULL);
  if (topic != NULL &&
      mgos_mqtt_num_unsent_bytes() < MGOS_MQTT_LOG_PUSHBACK_THRESHOLD) {
    static uint32_t s_seq = 0;
    char *msg = arg->buf;
    int msg_len = mg_asprintf(
        &msg, MGOS_DEBUG_TMP_BUF_SIZE, "%s %u %.3lf %d|%.*s",
        (mgos_sys_config_get_device_id() ? mgos_sys_config_get_device_id()
                                         : "-"),
        (unsigned int) s_seq, mg_time(), arg->fd, (int) arg->len,
        (const char *) arg->data);
    if (arg->len > 0) {
      mgos_mqtt_pub(topic, msg, msg_len, 0 /* qos */, false);
      s_seq++;
    }
    if (msg != arg->buf) free(msg);
  }

  (void) ev;
  (void) userdata;
}

static void mgos_mqtt_free_config(struct mgos_config_mqtt *cfg) {
  if (cfg == NULL) return;
  free(cfg->server);
  free(cfg->client_id);
  free(cfg->user);
  free(cfg->pass);
  free(cfg->ssl_cert);
  free(cfg->ssl_key);
  free(cfg->ssl_ca_cert);
  free(cfg->ssl_cipher_suites);
  free(cfg->will_topic);
  free(cfg->will_message);
  memset(cfg, 0, sizeof(*cfg));
  free(cfg);
}

bool mgos_mqtt_set_config(const struct mgos_config_mqtt *cfg) {
  bool ret = false;
  struct mgos_config_mqtt *new_cfg = NULL;
  if (!cfg->enable) {
    ret = true;
    goto out;
  }
  if (cfg->server == NULL) {
    LOG(LL_ERROR, ("MQTT requires server name"));
    goto out;
  }
  new_cfg = (struct mgos_config_mqtt *) calloc(1, sizeof(*new_cfg));
  if (new_cfg == NULL) goto out;
  new_cfg->enable = cfg->enable;
  new_cfg->server = strdup(cfg->server);
  if (cfg->client_id) new_cfg->client_id = strdup(cfg->client_id);
  if (cfg->user) new_cfg->user = strdup(cfg->user);
  if (cfg->pass) new_cfg->pass = strdup(cfg->pass);
  new_cfg->reconnect_timeout_min = cfg->reconnect_timeout_min;
  new_cfg->reconnect_timeout_max = cfg->reconnect_timeout_max;
  if (cfg->ssl_cert) new_cfg->ssl_cert = strdup(cfg->ssl_cert);
  if (cfg->ssl_key) new_cfg->ssl_key = strdup(cfg->ssl_key);
  if (cfg->ssl_ca_cert) new_cfg->ssl_ca_cert = strdup(cfg->ssl_ca_cert);
  if (cfg->ssl_cipher_suites) {
    new_cfg->ssl_cipher_suites = strdup(cfg->ssl_cipher_suites);
  }
  if (cfg->ssl_psk_identity) {
    new_cfg->ssl_psk_identity = strdup(cfg->ssl_psk_identity);
  }
  if (cfg->ssl_psk_key) new_cfg->ssl_psk_key = strdup(cfg->ssl_psk_key);
  new_cfg->clean_session = cfg->clean_session;
  new_cfg->keep_alive = cfg->keep_alive;
  if (cfg->will_topic) new_cfg->will_topic = strdup(cfg->will_topic);
  if (cfg->will_message) new_cfg->will_message = strdup(cfg->will_message);
  new_cfg->max_qos = cfg->max_qos;
  new_cfg->recv_mbuf_limit = cfg->recv_mbuf_limit;
  new_cfg->require_time = cfg->require_time;
  new_cfg->cloud_events = cfg->cloud_events;

  ret = true;

out:
  if (ret) {
    const struct mgos_config_mqtt *old_cfg = s_cfg;
    s_cfg = new_cfg;
    if (s_conn != NULL) {
      s_conn->flags |= MG_F_CLOSE_IMMEDIATELY;
      s_conn = NULL;
    }
    const struct mgos_config_mqtt *cfg0 = mgos_sys_config_get_mqtt();
    const struct mgos_config_mqtt *cfg1 = mgos_sys_config_get_mqtt1();
    if (old_cfg != NULL && old_cfg != cfg0 && old_cfg != cfg1) {
      mgos_mqtt_free_config((struct mgos_config_mqtt *) old_cfg);
    }
  } else {
    mgos_mqtt_free_config(new_cfg);
  }
  return ret;
}

static bool mgos_mqtt_time_ok(void) {
  if (s_cfg == NULL) return false;
  if (!s_cfg->require_time) return true;
  if (mg_time() < 1500000000) {
    LOG(LL_DEBUG, ("Time is not set, not connecting"));
    return false;
  }
  return true;
}

bool mgos_mqtt_init(void) {
  if (mgos_sys_config_get_debug_stdout_topic() != NULL) {
    char *stdout_topic = strdup(mgos_sys_config_get_debug_stdout_topic());
    mgos_expand_mac_address_placeholders(stdout_topic);
    mgos_sys_config_set_debug_stdout_topic(stdout_topic);
    free(stdout_topic);
  }
  if (mgos_sys_config_get_debug_stderr_topic() != NULL) {
    char *stderr_topic = strdup(mgos_sys_config_get_debug_stderr_topic());
    mgos_expand_mac_address_placeholders(stderr_topic);
    mgos_sys_config_set_debug_stderr_topic(stderr_topic);
    free(stderr_topic);
  }

  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, mgos_mqtt_net_ev, NULL);
  mgos_event_add_handler(MGOS_EVENT_LOG, s_debug_write_cb, NULL);

  s_cfg = mgos_sys_config_get_mqtt();
  return true;
}

bool mgos_mqtt_global_connect(void) {
  bool ret = true;
  char *server = NULL;
  const char *err_str = NULL;
  struct mg_mgr *mgr = mgos_get_mgr();
  struct mg_connect_opts opts;

  if (s_cfg == NULL || !s_cfg->enable || s_cfg->server == NULL) {
    return false;
  }

  /* If we're already connected, do nothing */
  if (s_conn != NULL) return true;

  if (!mgos_mqtt_time_ok()) {
    mqtt_global_reconnect();
    return false;
  }

  memset(&opts, 0, sizeof(opts));
  opts.error_string = &err_str;
#if MG_ENABLE_SSL
  opts.ssl_cert = s_cfg->ssl_cert;
  opts.ssl_key = s_cfg->ssl_key;
  opts.ssl_ca_cert = s_cfg->ssl_ca_cert;
  opts.ssl_cipher_suites = s_cfg->ssl_cipher_suites;
  opts.ssl_psk_identity = s_cfg->ssl_psk_identity;
  opts.ssl_psk_key = s_cfg->ssl_psk_key;
#endif
  if (strchr(s_cfg->server, ':') == NULL) {
    int port = (s_cfg->ssl_ca_cert != NULL ? 8883 : 1883);
    mg_asprintf(&server, 0, "%s:%d", s_cfg->server, port);
    if (server == NULL) return false;
  } else {
    server = strdup(s_cfg->server);
  }
  LOG(LL_INFO, ("MQTT connecting to %s", server));

  s_connected = false;
  s_conn = mg_connect_opt(mgr, server, mgos_mqtt_ev, NULL, opts);
  if (s_conn != NULL) {
    mg_set_protocol_mqtt(s_conn);
    s_conn->recv_mbuf_limit = s_cfg->recv_mbuf_limit;
  } else {
    LOG(LL_ERROR, ("Error: %s", err_str));
    mqtt_global_reconnect();
    ret = false;
  }
  free(server);
  return ret;
}

bool mgos_mqtt_global_is_connected(void) {
  return s_connected;
}

static void reconnect_timer_cb(void *user_data) {
  s_reconnect_timer_id = MGOS_INVALID_TIMER_ID;
  mgos_mqtt_global_connect();
  (void) user_data;
}

static void mqtt_switch_config(void) {
  const struct mgos_config_mqtt *cfg0 = mgos_sys_config_get_mqtt();
  const struct mgos_config_mqtt *cfg1 =
      (const struct mgos_config_mqtt *) mgos_sys_config_get_mqtt1();
  const struct mgos_config_mqtt *cfg;
  if (s_cfg == cfg0) {
    cfg = cfg1;
  } else if (s_cfg == cfg1) {
    cfg = cfg0;
  } else {
    /* User set a custom config - don't mess with it. */
    return;
  }
  if (cfg->enable) {
    s_cfg = cfg;
    s_reconnect_timeout_ms = s_cfg->reconnect_timeout_min * 1000;
  }
}

static void mqtt_global_reconnect(void) {
  int rt_ms;
  if (s_cfg == NULL || s_cfg->server == NULL) return;

  if (s_reconnect_timeout_ms >= s_cfg->reconnect_timeout_max * 1000) {
    mqtt_switch_config();
  }

  if (s_reconnect_timeout_ms <= 0) s_reconnect_timeout_ms = 1000;

  rt_ms = s_reconnect_timeout_ms * 2;

  if (rt_ms < s_cfg->reconnect_timeout_min * 1000) {
    rt_ms = s_cfg->reconnect_timeout_min * 1000;
  }
  if (rt_ms > s_cfg->reconnect_timeout_max * 1000) {
    rt_ms = s_cfg->reconnect_timeout_max * 1000;
  }
  s_reconnect_timeout_ms = rt_ms;
  /* Fuzz the time a little. */
  rt_ms = (int) mgos_rand_range(rt_ms * 0.9, rt_ms * 1.1);
  LOG(LL_INFO, ("MQTT connecting after %d ms", rt_ms));
  mgos_clear_timer(s_reconnect_timer_id);
  s_reconnect_timer_id = mgos_set_timer(rt_ms, 0, reconnect_timer_cb, NULL);
}

struct mg_connection *mgos_mqtt_get_global_conn(void) {
  return s_conn;
}

bool mgos_mqtt_pub(const char *topic, const void *message, size_t len, int qos,
                   bool retain) {
  struct mg_connection *c = mgos_mqtt_get_global_conn();
  int flags = MG_MQTT_QOS(adjust_qos(qos));
  if (retain) flags |= MG_MQTT_RETAIN;
  if (c == NULL || !s_connected) return false;
  LOG(LL_DEBUG, ("Publishing to %s @ %d%s (%d): [%.*s]", topic, qos,
                 (retain ? " (RETAIN)" : ""), (int) len, (int) len,
                 (const char *) message));
  mg_mqtt_publish(c, topic, mgos_mqtt_get_packet_id(), flags, message, len);
  return true;
}

bool mgos_mqtt_pubf(const char *topic, int qos, bool retain,
                    const char *json_fmt, ...) {
  bool res;
  va_list ap;
  va_start(ap, json_fmt);
  res = mgos_mqtt_pubv(topic, qos, retain, json_fmt, ap);
  va_end(ap);
  return res;
}

bool mgos_mqtt_pubv(const char *topic, int qos, bool retain,
                    const char *json_fmt, va_list ap) {
  bool res = false;
  char *msg = json_vasprintf(json_fmt, ap);
  if (msg != NULL) {
    res = mgos_mqtt_pub(topic, msg, strlen(msg), qos, retain);
    free(msg);
  }
  return res;
}

struct sub_data {
  sub_handler_t handler;
  void *user_data;
};

static void mqttsubtrampoline(struct mg_connection *c, int ev, void *ev_data,
                              void *user_data) {
  if (ev != MG_EV_MQTT_PUBLISH) return;
  struct sub_data *sd = (struct sub_data *) user_data;
  struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
  sd->handler(c, mm->topic.p, mm->topic.len, mm->payload.p, mm->payload.len,
              sd->user_data);
}

void mgos_mqtt_sub(const char *topic, sub_handler_t handler, void *user_data) {
  struct sub_data *sd = (struct sub_data *) malloc(sizeof(*sd));
  sd->handler = handler;
  sd->user_data = user_data;
  mgos_mqtt_global_subscribe(mg_mk_str(topic), mqttsubtrampoline, sd);
}

size_t mgos_mqtt_num_unsent_bytes(void) {
  return (s_conn != NULL ? s_conn->send_mbuf.len : 0);
}
