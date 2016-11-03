/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdbool.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "common/queue.h"
#include "fw/src/mg_mdns.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_timers.h"
#include "fw/src/mg_wifi.h"

#if MG_ENABLE_MQTT

static mg_event_handler_t s_user_handler = NULL;
static void *s_user_data = NULL;
static int s_reconnect_timeout = 0;
struct mg_connection *s_conn = NULL;

static bool mqtt_global_connect(void);

static void call_user_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (s_user_handler != NULL) {
    nc->user_data = s_user_data;
    s_user_handler(nc, ev, ev_data);
  }
}

static void reconnect_timer_cb(void *user_data) {
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

      /* Schedule reconnect after a timeout */
      if (s_reconnect_timeout > smcfg->reconnect_timeout_max) {
        s_reconnect_timeout = smcfg->reconnect_timeout_max;
      }
      LOG(LL_DEBUG,
          ("MQTT reconnecting after %d seconds", s_reconnect_timeout));
      mg_set_c_timer(s_reconnect_timeout * 1000 /* ms */, 0, reconnect_timer_cb,
                     NULL);

      /*
       * If that connection fails, next reconnect timeout will be larger
       * (but not larger than reconnect_timeout_max from the config)
       */
      s_reconnect_timeout = s_reconnect_timeout * 2;
      break;
    }

    /* Delegate all MQTT events to the user's handler */
    case MG_EV_MQTT_CONNECT:
    case MG_EV_MQTT_CONNACK:
    case MG_EV_MQTT_PUBLISH:
    case MG_EV_MQTT_PUBACK:
    case MG_EV_MQTT_PUBREC:
    case MG_EV_MQTT_PUBREL:
    case MG_EV_MQTT_PUBCOMP:
    case MG_EV_MQTT_SUBSCRIBE:
    case MG_EV_MQTT_SUBACK:
    case MG_EV_MQTT_UNSUBSCRIBE:
    case MG_EV_MQTT_UNSUBACK:
    case MG_EV_MQTT_PINGREQ:
    case MG_EV_MQTT_PINGRESP:
    case MG_EV_MQTT_DISCONNECT:
      call_user_handler(nc, ev, ev_data);
      break;
  }
}

enum mg_init_result mg_mqtt_global_init(void) {
  enum mg_init_result ret = MG_INIT_OK;
  const struct sys_config_mqtt *smcfg = &get_cfg()->mqtt;
  s_reconnect_timeout = smcfg->reconnect_timeout_min;
  if (!mqtt_global_connect()) {
    ret = MG_INIT_MQTT_FAILED;
  }
  return ret;
}

static bool mqtt_global_connect(void) {
  bool ret = true;
  struct mg_mgr *mgr = mg_get_mgr();
  const struct sys_config *scfg = get_cfg();

  if (scfg->mqtt.server != NULL) {
    LOG(LL_INFO, ("MQTT connecting to %s", scfg->mqtt.server));

    struct mg_connection *nc = mg_connect(mgr, scfg->mqtt.server, ev_handler);
    if (nc == NULL) {
      ret = false;
    } else {
      struct mg_send_mqtt_handshake_opts opts;
      memset(&opts, 0, sizeof(opts));

      opts.user_name = scfg->device.id;
      opts.password = scfg->device.password;

      mg_set_protocol_mqtt(nc);
      mg_send_mqtt_handshake_opt(nc, "dummy", opts);
    }
  } else {
    LOG(LL_INFO, ("MQTT server address is empty => MQTT is disabled."));
  }

  return ret;
}

void mg_mqtt_set_global_handler(mg_event_handler_t handler, void *ud) {
  s_user_handler = handler;
  s_user_data = ud;
}

struct mg_connection *mg_mqtt_get_global_conn(void) {
  return s_conn;
}

#endif /* MG_ENABLE_MQTT */
