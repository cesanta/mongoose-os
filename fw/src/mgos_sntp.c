/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_sntp.h"

#include <stdbool.h>
#include <stdlib.h>

#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

struct time_change_cb {
  mgos_sntp_time_change_cb cb;
  void *cb_arg;
  SLIST_ENTRY(time_change_cb) entries;
};

struct mgos_sntp_state {
  struct mg_connection *nc;
  bool synced;
  int retry_timeout_ms;
  mgos_timer_id retry_timer_id;
  SLIST_HEAD(time_change_cbs, time_change_cb) time_change_cbs;
};

static struct mgos_sntp_state s_state;
static void mgos_sntp_retry(void);

static void mgos_sntp_ev(struct mg_connection *nc, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_CONNECT: {
      LOG(LL_DEBUG, ("SNTP query sent"));
      mg_sntp_send_request(nc);
      break;
    }
    case MG_SNTP_REPLY: {
      struct mg_sntp_message *m = (struct mg_sntp_message *) ev_data;
      double now = mg_time();
      double delta = (m->time - now);
      LOG(LL_INFO,
          ("SNTP reply: time %lf, local %lf, delta %lf", m->time, now, delta));
      struct timeval tv;
      tv.tv_sec = (time_t) m->time;
      tv.tv_usec = (m->time - tv.tv_sec) * 1000000;
      if (settimeofday(&tv, NULL) == 0) {
        struct time_change_cb *tccb;
        SLIST_FOREACH(tccb, &s_state.time_change_cbs, entries) {
          tccb->cb(tccb->cb_arg, delta);
        }
      } else {
        LOG(LL_ERROR, ("Failed to set time"));
      }
      s_state.retry_timeout_ms = 0;
      s_state.synced = true;
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      if (s_state.retry_timer_id != MGOS_INVALID_TIMER_ID) {
        mgos_clear_timer(s_state.retry_timer_id);
        s_state.retry_timer_id = MGOS_INVALID_TIMER_ID;
      }
      break;
    }
    case MG_SNTP_MALFORMED_REPLY:
    case MG_SNTP_FAILED:
      LOG(LL_ERROR, ("SNTP error: %d", ev));
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    case MG_EV_CLOSE:
      LOG(LL_DEBUG, ("SNTP close"));
      if (s_state.nc == nc) {
        s_state.nc = NULL;
        mgos_sntp_retry();
      }
      break;
  }
  (void) nc;
}

static bool mgos_sntp_query(const char *server) {
  if (s_state.nc != NULL) {
    s_state.nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return false;
  }
  s_state.nc = mg_sntp_connect(mgos_get_mgr(), mgos_sntp_ev, server);
  LOG(LL_INFO, ("SNTP query to %s", server));
  return (s_state.nc != NULL);
}

static void mgos_sntp_retry_timer_cb(void *user_data) {
  const struct sys_config_sntp *scfg = &get_cfg()->sntp;
  s_state.retry_timer_id = MGOS_INVALID_TIMER_ID;
  mgos_sntp_query(scfg->server);
  /*
   * Response may never arrive, so we schedule a retry immediately.
   * Successful response will clear the timer.
   */
  mgos_sntp_retry();
  (void) user_data;
}

static void mgos_sntp_retry(void) {
  const struct sys_config_sntp *scfg = &get_cfg()->sntp;
  if (!scfg->enable) return;
  if (s_state.retry_timer_id != MGOS_INVALID_TIMER_ID) return;
  int rt_ms = 0;
  if (s_state.synced) {
    rt_ms = scfg->update_interval * 1000;
  } else {
    rt_ms = s_state.retry_timeout_ms * 2;
    if (rt_ms < scfg->retry_min * 1000) {
      rt_ms = scfg->retry_min * 1000;
    }
    if (rt_ms > scfg->retry_max * 1000) {
      rt_ms = scfg->retry_max * 1000;
    }
    s_state.retry_timeout_ms = rt_ms;
  }
  rt_ms = (int) mgos_rand_range(rt_ms * 0.9, rt_ms * 1.1);
  LOG(LL_DEBUG, ("SNTP next query in %d ms", rt_ms));
  s_state.retry_timer_id =
      mgos_set_timer(rt_ms, 0, mgos_sntp_retry_timer_cb, NULL);
}

void mgos_sntp_add_time_change_cb(mgos_sntp_time_change_cb cb, void *arg) {
  struct time_change_cb *tccb =
      (struct time_change_cb *) calloc(1, sizeof(*tccb));
  tccb->cb = cb;
  tccb->cb_arg = arg;
  SLIST_INSERT_HEAD(&s_state.time_change_cbs, tccb, entries);
}

#if MGOS_ENABLE_WIFI
static void mgos_sntp_wifi_ready(enum mgos_wifi_status event, void *arg) {
  if (event != MGOS_WIFI_IP_ACQUIRED) return;

  mgos_sntp_retry();
  (void) arg;
}
#endif

enum mgos_init_result mgos_sntp_init(void) {
  struct sys_config_sntp *scfg = &get_cfg()->sntp;
  if (!scfg->enable) return MGOS_INIT_OK;
  if (scfg->server == NULL) {
    LOG(LL_ERROR, ("sntp.server is required"));
    return MGOS_INIT_SNTP_INIT_FAILED;
  }
#if MGOS_ENABLE_WIFI
  mgos_wifi_add_on_change_cb(mgos_sntp_wifi_ready, NULL);
#else
  mgos_sntp_retry();
#endif
  return MGOS_INIT_OK;
}
