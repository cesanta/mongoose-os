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
#include "fw/src/miot_mdns.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"
#include "fw/src/miot_wifi.h"

#if MIOT_ENABLE_MQTT

struct topic_handler {
  struct mg_str topic;
  mg_event_handler_t handler;
  void *user_data;
  uint16_t sub_id;
  SLIST_ENTRY(topic_handler) entries;
};

static mg_event_handler_t s_user_handler = NULL;
static void *s_user_data = NULL;
static int s_reconnect_timeout = 0;
static miot_timer_id reconnect_timer_id = MIOT_INVALID_TIMER_ID;
static struct mg_connection *s_conn = NULL;

SLIST_HEAD(topic_handlers, topic_handler) s_topic_handlers;

static bool mqtt_global_connect(void);

static bool call_topic_handler(struct mg_connection *nc, int ev,
                               void *ev_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
  struct topic_handler *th;
  SLIST_FOREACH(th, &s_topic_handlers, entries) {
    if ((ev == MG_EV_CLOSE) ||
        (ev == MG_EV_MQTT_SUBACK && th->sub_id == msg->message_id) ||
        mg_strcmp(th->topic, msg->topic) == 0) {
      nc->user_data = th->user_data;
      th->handler(nc, ev, ev_data);
      return true;
    }
  }
  return false;
}

static void call_global_handler(struct mg_connection *nc, int ev,
                                void *ev_data) {
  if (s_user_handler != NULL) {
    nc->user_data = s_user_data;
    s_user_handler(nc, ev, ev_data);
  }
}

static void reconnect_timer_cb(void *user_data) {
  reconnect_timer_id = MIOT_INVALID_TIMER_ID;
  mqtt_global_connect();

  (void) user_data;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  const struct sys_config_mqtt *smcfg = &get_cfg()->mqtt;

  switch (ev) {
    case MG_EV_CONNECT: {
      int success = (*(int *) ev_data == 0);
      LOG(LL_INFO, ("MQTT Connect (%d)", success));
      if (success) {
        s_conn = nc;
        s_reconnect_timeout = smcfg->reconnect_timeout_min;
      }
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("MQTT Disconnect"));
      s_conn = NULL;
      call_topic_handler(nc, ev, NULL);

      /* Schedule reconnect after a timeout */
      if (s_reconnect_timeout > smcfg->reconnect_timeout_max) {
        s_reconnect_timeout = smcfg->reconnect_timeout_max;
      }
      LOG(LL_DEBUG,
          ("MQTT reconnecting after %d seconds", s_reconnect_timeout));
      reconnect_timer_id = miot_set_timer(s_reconnect_timeout * 1000 /* ms */,
                                          0, reconnect_timer_cb, NULL);

      /*
       * If that connection fails, next reconnect timeout will be larger
       * (but not larger than reconnect_timeout_max from the config)
       */
      s_reconnect_timeout = s_reconnect_timeout * 2;
      break;
    }
    case MG_EV_POLL: {
      struct mg_mqtt_proto_data *pd =
          (struct mg_mqtt_proto_data *) nc->proto_data;
      double now = mg_time();
      if ((now - nc->last_io_time) > pd->keep_alive / 2) {
        LOG(LL_DEBUG, ("Send PINGREQ"));
        mg_mqtt_ping(nc);
        nc->last_io_time = (time_t) mg_time();
      }
      break;
    }
    case MG_EV_MQTT_PINGRESP: {
      /* Do nothing here */
      LOG(LL_DEBUG, ("Got PINGRESP"));
      break;
    }
    case MG_EV_MQTT_CONNACK: {
      struct topic_handler *th;
      uint16_t sub_id = 1;
      SLIST_FOREACH(th, &s_topic_handlers, entries) {
        struct mg_mqtt_topic_expression te = {.topic = th->topic.p, .qos = 0};
        th->sub_id = sub_id++;
        mg_mqtt_subscribe(nc, &te, 1, th->sub_id);
        LOG(LL_DEBUG, ("Subscribing to '%s'", te.topic));
      }
      call_global_handler(nc, ev, ev_data);
      break;
    }
    /* Delegate almost all MQTT events to the user's handler */
    case MG_EV_MQTT_SUBACK:
    case MG_EV_MQTT_PUBLISH:
      if (call_topic_handler(nc, ev, ev_data)) break;
    /* fall through */
    case MG_EV_MQTT_PUBACK:
    case MG_EV_MQTT_CONNECT:
    case MG_EV_MQTT_PUBREC:
    case MG_EV_MQTT_PUBREL:
    case MG_EV_MQTT_PUBCOMP:
    case MG_EV_MQTT_SUBSCRIBE:
    case MG_EV_MQTT_UNSUBSCRIBE:
    case MG_EV_MQTT_UNSUBACK:
    case MG_EV_MQTT_PINGREQ:
    case MG_EV_MQTT_DISCONNECT:
      call_global_handler(nc, ev, ev_data);
      break;
  }
}

void miot_mqtt_global_subscribe(const struct mg_str topic,
                                mg_event_handler_t handler, void *ud) {
  struct topic_handler *th = (struct topic_handler *) calloc(1, sizeof(*th));
  th->topic.p = (char *) calloc(topic.len + 1 /* + NUL */, 1);
  memcpy((char *) th->topic.p, topic.p, topic.len);
  th->topic.len = topic.len;
  th->handler = handler;
  th->user_data = ud;
  SLIST_INSERT_HEAD(&s_topic_handlers, th, entries);
}

#if MIOT_ENABLE_WIFI
static void mg_mqtt_wifi_ready(enum miot_wifi_status event, void *arg) {
  if (event != MIOT_WIFI_IP_ACQUIRED) return;

  mqtt_global_connect();
  (void) arg;
}
#endif

enum miot_init_result miot_mqtt_global_init(void) {
  enum miot_init_result ret = MIOT_INIT_OK;
  const struct sys_config_mqtt *smcfg = &get_cfg()->mqtt;
  s_reconnect_timeout = smcfg->reconnect_timeout_min;
#if MIOT_ENABLE_WIFI
  miot_wifi_add_on_change_cb(mg_mqtt_wifi_ready, NULL);
#else
  mqtt_global_connect();
#endif
  return ret;
}

static bool mqtt_global_connect(void) {
  bool ret = true;
  struct mg_mgr *mgr = miot_get_mgr();
  const struct sys_config *scfg = get_cfg();
  struct mg_connect_opts opts;

  /* If we're already connected, do nothing */
  if (s_conn != NULL) return ret;

  if (reconnect_timer_id != MIOT_INVALID_TIMER_ID) {
    miot_clear_timer(reconnect_timer_id);
    reconnect_timer_id = MIOT_INVALID_TIMER_ID;
  }

  memset(&opts, 0, sizeof(opts));

  if (scfg->mqtt.server != NULL && scfg->device.id != NULL) {
    LOG(LL_INFO, ("MQTT connecting to %s", scfg->mqtt.server));

#if MG_ENABLE_SSL
    opts.ssl_cert = scfg->mqtt.ssl_cert;
    opts.ssl_key = scfg->mqtt.ssl_key;
    opts.ssl_ca_cert = scfg->mqtt.ssl_ca_cert;
#endif

    struct mg_connection *nc =
        mg_connect_opt(mgr, scfg->mqtt.server, ev_handler, opts);
    if (nc == NULL) {
      ret = false;
    } else {
      struct mg_send_mqtt_handshake_opts opts;
      memset(&opts, 0, sizeof(opts));

      opts.user_name = scfg->device.id;
      opts.password = scfg->device.password;
      if (scfg->mqtt.clean_session) {
        opts.flags |= MG_MQTT_CLEAN_SESSION;
      }
      opts.keep_alive = scfg->mqtt.keep_alive;
      opts.will_topic = scfg->mqtt.will_topic;
      opts.will_message = scfg->mqtt.will_message;

      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake_opt(nc, scfg->device.id, opts);
    }
  }

  return ret;
}

void miot_mqtt_set_global_handler(mg_event_handler_t handler, void *ud) {
  s_user_handler = handler;
  s_user_data = ud;
}

struct mg_connection *miot_mqtt_get_global_conn(void) {
  return s_conn;
}

#endif /* MIOT_ENABLE_MQTT */
