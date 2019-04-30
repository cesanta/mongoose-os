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

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_event.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_sys_config.h"
#include "mgos_time.h"
#include "mgos_timers.h"
#include "mgos_utils.h"

struct mgos_sntp_state {
  struct mg_connection *nc;
  bool synced;
  int retry_timeout_ms;
  mgos_timer_id retry_timer_id;
};

static struct mgos_sntp_state s_state;
static void mgos_sntp_retry(void);

static void mgos_sntp_ev(struct mg_connection *nc, int ev, void *ev_data,
                         void *user_data) {
  char addr[32];
  switch (ev) {
    case MG_EV_CONNECT: {
      if (*((int *) ev_data) == 0) {
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
        LOG(LL_DEBUG, ("SNTP sent query to %s", addr));
        mg_sntp_send_request(nc);
      }
      break;
    }
    case MG_SNTP_REPLY: {
      struct mg_sntp_message *m = (struct mg_sntp_message *) ev_data;
      double now = mg_time();
      double delta = (m->time - now);
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
      LOG(LL_INFO, ("SNTP reply from %s: time %lf, local %lf, delta %lf", addr,
                    m->time, now, delta));
      if (mgos_settimeofday(m->time, NULL) != 0) {
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
  (void) user_data;
}

static bool mgos_sntp_query(const char *server) {
  if (s_state.nc != NULL) {
    s_state.nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return false;
  }
  s_state.nc = mg_sntp_connect(mgos_get_mgr(), mgos_sntp_ev, NULL, server);
  LOG(LL_INFO, ("SNTP query to %s", server));
  return (s_state.nc != NULL);
}

static void mgos_sntp_retry_timer_cb(void *user_data) {
  s_state.retry_timer_id = MGOS_INVALID_TIMER_ID;
  mgos_sntp_query(mgos_sys_config_get_sntp_server());
  /*
   * Response may never arrive, so we schedule a retry immediately.
   * Successful response will clear the timer.
   */
  mgos_sntp_retry();
  (void) user_data;
}

static void mgos_sntp_retry(void) {
  if (!mgos_sys_config_get_sntp_enable()) return;
  if (s_state.retry_timer_id != MGOS_INVALID_TIMER_ID) return;
  int rt_ms = 0;
  if (s_state.synced) {
    rt_ms = mgos_sys_config_get_sntp_update_interval() * 1000;
  } else {
    rt_ms = s_state.retry_timeout_ms * 2;
    if (rt_ms < mgos_sys_config_get_sntp_retry_min() * 1000) {
      rt_ms = mgos_sys_config_get_sntp_retry_min() * 1000;
    }
    if (rt_ms > mgos_sys_config_get_sntp_retry_max() * 1000) {
      rt_ms = mgos_sys_config_get_sntp_retry_max() * 1000;
    }
    s_state.retry_timeout_ms = rt_ms;
  }
  rt_ms = (int) mgos_rand_range(rt_ms * 0.9, rt_ms * 1.1);
  LOG(LL_DEBUG, ("SNTP next query in %d ms", rt_ms));
  s_state.retry_timer_id =
      mgos_set_timer(rt_ms, 0, mgos_sntp_retry_timer_cb, NULL);
}

static void mgos_time_change_cb(int ev, void *evd, void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mgos_time_changed_arg *ev_data = (struct mgos_time_changed_arg *) evd;
  struct mg_connection *nc;
  for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
    if (nc->ev_timer_time > 0) {
      nc->ev_timer_time += ev_data->delta;
    }
  }

  (void) ev;
}

static void mgos_sntp_net_ev(int ev, void *evd, void *arg) {
  if (ev != MGOS_NET_EV_IP_ACQUIRED) return;
  mgos_sntp_retry();
  (void) evd;
  (void) arg;
}

bool mgos_sntp_init(void) {
  if (!mgos_sys_config_get_sntp_enable()) return true;
  if (mgos_sys_config_get_sntp_server() == NULL) {
    LOG(LL_ERROR, ("sntp.server is required"));
    return false;
  }

  mgos_event_add_handler(MGOS_EVENT_TIME_CHANGED, mgos_time_change_cb,
                         mgos_get_mgr());
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, mgos_sntp_net_ev, NULL);
  return true;
}
