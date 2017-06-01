/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdbool.h>

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "common/platform.h"
#include "common/queue.h"
#include "fw/src/mgos_mdns.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_mqtt.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#if MGOS_ENABLE_MQTT

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
static mgos_mqtt_auth_callback_t s_auth_cb = NULL;
static void *s_auth_cb_arg = NULL;

SLIST_HEAD(topic_handlers, topic_handler) s_topic_handlers;
SLIST_HEAD(global_handlers, global_handler) s_global_handlers;

static void mqtt_global_reconnect(void);

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
      if (ev == MG_EV_MQTT_PUBLISH && th->qos > 0) {
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

static void mgos_mqtt_ev(struct mg_connection *nc, int ev, void *ev_data,
                         void *user_data) {
  if (ev > MG_MQTT_EVENT_BASE) {
    LOG(LL_DEBUG, ("MQTT event: %d", ev));
  }

  switch (ev) {
    case MG_EV_CONNECT: {
      bool success = (*(int *) ev_data == 0);
      LOG(LL_INFO, ("MQTT Connect (%d)", success));
      if (!success) break;
      struct mg_send_mqtt_handshake_opts opts;
      const struct sys_config *cfg = get_cfg();
      const struct sys_config_mqtt *mcfg = &cfg->mqtt;
      memset(&opts, 0, sizeof(opts));
      char *cb_client_id = NULL, *cb_user = NULL, *cb_pass = NULL;
      if (s_auth_cb != NULL) {
        s_auth_cb(&cb_client_id, &cb_user, &cb_pass, s_auth_cb_arg);
        opts.user_name = cb_user;
        opts.password = cb_pass;
      } else {
        opts.user_name = mcfg->user;
        opts.password = mcfg->pass;
      }
      if (mcfg->clean_session) {
        opts.flags |= MG_MQTT_CLEAN_SESSION;
      }
      opts.keep_alive = mcfg->keep_alive;
      opts.will_topic = mcfg->will_topic;
      opts.will_message = mcfg->will_message;
      const char *client_id =
          (cb_client_id != NULL
               ? cb_client_id
               : (mcfg->client_id != NULL ? mcfg->client_id : cfg->device.id));
      mg_send_mqtt_handshake_opt(nc, client_id, opts);
      free(cb_client_id);
      free(cb_user);
      free(cb_pass);
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("MQTT Disconnect"));
      s_conn = NULL;
      s_connected = false;
      call_global_handlers(nc, ev, NULL, user_data);
      mqtt_global_reconnect();
      break;
    }
    case MG_EV_POLL: {
      struct mg_mqtt_proto_data *pd =
          (struct mg_mqtt_proto_data *) nc->proto_data;
      double now = mg_time();
      if (pd->keep_alive > 0 && (now - nc->last_io_time) > pd->keep_alive) {
        LOG(LL_DEBUG, ("Send PINGREQ"));
        mg_mqtt_ping(nc);
        nc->last_io_time = (time_t) mg_time();
      }
      call_global_handlers(nc, ev, NULL, user_data);
      break;
    }
    case MG_EV_MQTT_PINGRESP: {
      /* Do nothing here */
      LOG(LL_DEBUG, ("Got PINGRESP"));
      break;
    }
    case MG_EV_MQTT_CONNACK: {
      struct topic_handler *th;
      int code = ((struct mg_mqtt_message *) ev_data)->connack_ret_code;
      LOG((code == 0 ? LL_INFO : LL_ERROR), ("MQTT CONNACK %d", code));
      if (code == 0) {
        s_connected = true;
        s_reconnect_timeout_ms = 0;
        call_global_handlers(nc, ev, ev_data, user_data);
        SLIST_FOREACH(th, &s_topic_handlers, entries) {
          struct mg_mqtt_topic_expression te = {.topic = th->topic.p,
                                                .qos = th->qos};
          th->sub_id = mgos_mqtt_get_packet_id();
          mg_mqtt_subscribe(nc, &te, 1 /* len */, th->sub_id);
          LOG(LL_INFO, ("Subscribing to '%s'", te.topic));
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
  th->qos = 1;
  SLIST_INSERT_HEAD(&s_topic_handlers, th, entries);
}

void mgos_mqtt_add_global_handler(mg_event_handler_t handler, void *ud) {
  struct global_handler *gh = (struct global_handler *) calloc(1, sizeof(*gh));
  gh->handler = handler;
  gh->user_data = ud;
  SLIST_INSERT_HEAD(&s_global_handlers, gh, entries);
}

void mgos_mqtt_set_auth_callback(mgos_mqtt_auth_callback_t cb, void *cb_arg) {
  s_auth_cb = cb;
  s_auth_cb_arg = cb_arg;
}

#if MGOS_ENABLE_WIFI
static void mgos_mqtt_wifi_ready(enum mgos_wifi_status event, void *arg) {
  if (event != MGOS_WIFI_IP_ACQUIRED) return;

  mqtt_global_reconnect();
  (void) arg;
}
#endif

enum mgos_init_result mgos_mqtt_init(void) {
  const struct sys_config_mqtt *mcfg = &get_cfg()->mqtt;
  if (!mcfg->enable) return MGOS_INIT_OK;
  if (mcfg->server == NULL) {
    LOG(LL_ERROR, ("MQTT requires server name"));
    return MGOS_INIT_MQTT_INIT_FAILED;
  }
#if MGOS_ENABLE_WIFI
  mgos_wifi_add_on_change_cb(mgos_mqtt_wifi_ready, NULL);
#else
  mqtt_global_reconnect();
#endif
  return MGOS_INIT_OK;
}

static bool mqtt_global_connect(void) {
  bool ret = true;
  struct mg_mgr *mgr = mgos_get_mgr();
  const struct sys_config *scfg = get_cfg();
  struct mg_connect_opts opts;

  /* If we're already connected, do nothing */
  if (s_conn != NULL) return ret;

  if (!scfg->mqtt.enable) {
    return false;
  }

  LOG(LL_INFO, ("MQTT connecting to %s", scfg->mqtt.server));
  memset(&opts, 0, sizeof(opts));
#if MG_ENABLE_SSL
  opts.ssl_cert = scfg->mqtt.ssl_cert;
  opts.ssl_key = scfg->mqtt.ssl_key;
  opts.ssl_ca_cert = scfg->mqtt.ssl_ca_cert;
  opts.ssl_cipher_suites = scfg->mqtt.ssl_cipher_suites;
  opts.ssl_psk_identity = scfg->mqtt.ssl_psk_identity;
  opts.ssl_psk_key = scfg->mqtt.ssl_psk_key;
#endif

  s_connected = false;
  s_conn = mg_connect_opt(mgr, scfg->mqtt.server, mgos_mqtt_ev, NULL, opts);
  if (s_conn != NULL) {
    mg_set_protocol_mqtt(s_conn);
  } else {
    ret = false;
  }
  return ret;
}

static void reconnect_timer_cb(void *user_data) {
  s_reconnect_timer_id = MGOS_INVALID_TIMER_ID;
  if (!mqtt_global_connect()) {
    mqtt_global_reconnect();
  }
  (void) user_data;
}

static void mqtt_global_reconnect(void) {
  const struct sys_config_mqtt *smcfg = &get_cfg()->mqtt;
  int rt_ms = s_reconnect_timeout_ms * 2;
  if (smcfg->server == NULL) return;

  if (rt_ms < smcfg->reconnect_timeout_min * 1000) {
    rt_ms = smcfg->reconnect_timeout_min * 1000;
  }
  if (rt_ms > smcfg->reconnect_timeout_max * 1000) {
    rt_ms = smcfg->reconnect_timeout_max * 1000;
  }
  /* Fuzz the time a little. */
  rt_ms = (int) mgos_rand_range(rt_ms * 0.9, rt_ms * 1.1);
  LOG(LL_INFO, ("MQTT connecting after %d ms", rt_ms));
  s_reconnect_timeout_ms = rt_ms;
  if (s_reconnect_timer_id != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(s_reconnect_timer_id);
  }
  s_reconnect_timer_id = mgos_set_timer(rt_ms, 0, reconnect_timer_cb, NULL);
}

struct mg_connection *mgos_mqtt_get_global_conn(void) {
  return s_conn;
}

bool mgos_mqtt_pub(const char *topic, const void *message, size_t len,
                   int qos) {
  struct mg_connection *c = mgos_mqtt_get_global_conn();
  if (c == NULL || !s_connected) return false;
  LOG(LL_DEBUG, ("Publishing to %s @ %d (%d): [%.*s]", topic, qos, (int) len,
                 (int) len, (const char *) message));
  mg_mqtt_publish(c, topic, mgos_mqtt_get_packet_id(), MG_MQTT_QOS(qos),
                  message, len);
  return true;
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
  if (s_connected) s_conn->flags |= MG_F_CLOSE_IMMEDIATELY;  // Reconnect
}

size_t mgos_mqtt_num_unsent_bytes(void) {
  return (s_conn != NULL ? s_conn->send_mbuf.len : 0);
}

#endif /* MGOS_ENABLE_MQTT */
