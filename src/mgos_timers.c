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

#include "mgos_timers_internal.h"

#include "common/queue.h"

#include "mgos_event.h"
#include "mgos_features.h"
#include "mgos_mongoose.h"
#include "mgos_mongoose_internal.h"
#include "mgos_system.h"
#include "mgos_time.h"

#if MGOS_NUM_HW_TIMERS > 0
#ifdef __LP64__
#define MGOS_SW_TIMER_MASK 0xffff000000000000
#else
#define MGOS_SW_TIMER_MASK 0xffff0000
#endif
#else
/* All timers are soft timers. */
#define MGOS_SW_TIMER_MASK ((uintptr_t) -1)
#endif

#ifndef IRAM
#define IRAM
#endif

struct timer_info {
  int interval_ms;
  double next_invocation;
  timer_callback cb;
  void *cb_arg;
  LIST_ENTRY(timer_info) entries;
};

struct timer_data {
  struct timer_info *current;
  struct mg_connection *nc;
  LIST_HEAD(timers, timer_info) timers;
};

static struct timer_data *s_timer_data = NULL;
static struct mgos_rlock_type *s_timer_data_lock = NULL;

static void schedule_next_timer(struct timer_data *td, double now) {
  struct timer_info *ti;
  struct timer_info *min_ti = NULL;
  LIST_FOREACH(ti, &td->timers, entries) {
    if (min_ti == NULL || ti->next_invocation < min_ti->next_invocation) {
      min_ti = ti;
    }
  }
  td->current = min_ti;
  if (min_ti != NULL) {
    double diff = min_ti->next_invocation - now;
    td->nc->ev_timer_time = mg_time() + diff;
  } else {
    td->nc->ev_timer_time = 0;
  }
}

static void mgos_timer_ev(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  if (ev != MG_EV_TIMER) return;
  timer_callback cb = NULL;
  void *cb_arg = NULL;
  {
    mgos_rlock(s_timer_data_lock);
    struct timer_data *td = (struct timer_data *) user_data;
    struct timer_info *ti = td->current;
    /* Current can be NULL if it was the first to fire but was cleared. */
    const double now = mgos_uptime();
    if (ti != NULL && ti->next_invocation <= now) {
      cb = ti->cb;
      cb_arg = ti->cb_arg;
      if (ti->interval_ms >= 0) {
        const double intvl = (ti->interval_ms / 1000.0);
        ti->next_invocation += intvl;
        /* Polling loop was delayed, re-sync the invocation. */
        if (ti->next_invocation < now) ti->next_invocation = now + intvl;
        ti = NULL;
      } else {
        LIST_REMOVE(ti, entries);
      }
    } else {
      ti = NULL;
    }
    schedule_next_timer(td, now);
    mgos_runlock(s_timer_data_lock);
    if (ti != NULL) free(ti);
  }
  if (cb != NULL) cb(cb_arg);
  (void) ev_data;
  (void) nc;
}

mgos_timer_id mgos_set_timer(int msecs, int flags, timer_callback cb,
                             void *arg) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MGOS_INVALID_TIMER_ID;
  if (flags & MGOS_TIMER_REPEAT) {
    ti->interval_ms = msecs;
  } else {
    ti->interval_ms = -1;
  }
  double now = mgos_uptime();
  if (flags & MGOS_TIMER_RUN_NOW) {
    ti->next_invocation = 1;
  } else {
    ti->next_invocation = now + msecs / 1000.0;
  }
  ti->cb = cb;
  ti->cb_arg = arg;
  {
    mgos_rlock(s_timer_data_lock);
    LIST_INSERT_HEAD(&s_timer_data->timers, ti, entries);
    schedule_next_timer(s_timer_data, now);
    mgos_runlock(s_timer_data_lock);
  }
  mongoose_schedule_poll(false /* from_isr */);
  return (mgos_timer_id) ti;
}

static void mgos_clear_sw_timer(mgos_timer_id id) {
  struct timer_info *ti = (struct timer_info *) id, *ti2;
  mgos_rlock(s_timer_data_lock);
  LIST_FOREACH(ti2, &s_timer_data->timers, entries) {
    if (ti2 == ti) break;
  }
  if (ti2 == NULL) {
    /* Not a valid timer */
    mgos_runlock(s_timer_data_lock);
    return;
  }
  LIST_REMOVE(ti, entries);
  if (s_timer_data->current == ti) {
    schedule_next_timer(s_timer_data, mgos_uptime());
    /* Removing a timer can only push back invocation, no need to do a poll. */
  }
  mgos_runlock(s_timer_data_lock);
  free(ti);
}

void mgos_clear_hw_timer(mgos_timer_id id);

IRAM void mgos_clear_timer(mgos_timer_id id) {
  if (id == MGOS_INVALID_TIMER_ID) {
    return;
  } else if (id & MGOS_SW_TIMER_MASK) {
    mgos_clear_sw_timer(id);
  } else {
    mgos_clear_hw_timer(id);
  }
}

bool mgos_get_timer_info(mgos_timer_id id, struct mgos_timer_info *info) {
  struct timer_info *ti = (struct timer_info *) id, *ti2;
  mgos_rlock(s_timer_data_lock);
  LIST_FOREACH(ti2, &s_timer_data->timers, entries) {
    if (ti2 == ti) break;
  }
  if (ti2 == NULL) {
    /* Not a valid timer */
    mgos_runlock(s_timer_data_lock);
    return false;
  }
  info->interval_ms = ti->interval_ms;
  info->msecs_left = (ti->next_invocation - mgos_uptime()) * 1000;
  info->cb = ti->cb;
  info->cb_arg = ti->cb_arg;
  mgos_runlock(s_timer_data_lock);
  return true;
}

static void mgos_poll_cb(void *arg) {
  struct timer_data *td = (struct timer_data *) arg;
  mgos_rlock(s_timer_data_lock);
  schedule_next_timer(td, mgos_uptime());
  mgos_runlock(s_timer_data_lock);
}

static void mgos_time_change_cb(int ev, void *evd, void *arg) {
  mgos_poll_cb(arg);
  (void) evd;
  (void) ev;
}

enum mgos_init_result mgos_hw_timers_init(void);

enum mgos_init_result mgos_timers_init(void) {
  struct timer_data *td = (struct timer_data *) calloc(1, sizeof(*td));
  struct mg_add_sock_opts opts;
  memset(&opts, 0, sizeof(opts));
  td->nc =
      mg_add_sock_opt(mgos_get_mgr(), INVALID_SOCKET, mgos_timer_ev, td, opts);
  if (td->nc == NULL) {
    return MGOS_INIT_TIMERS_INIT_FAILED;
  }
  s_timer_data = td;
  s_timer_data_lock = mgos_rlock_create();
  mgos_event_add_handler(MGOS_EVENT_TIME_CHANGED, mgos_time_change_cb, td);
  mgos_add_poll_cb(mgos_poll_cb, td);
  return mgos_hw_timers_init();
}
