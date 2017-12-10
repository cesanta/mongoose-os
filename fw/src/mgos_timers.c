/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_timers_internal.h"

#include "common/queue.h"

#include <mgos_features.h>
#include <mgos_hal.h>
#include <mgos_mongoose.h>
#include <mgos_sntp.h>

#define MGOS_SW_TIMER_MASK 0xffff0000

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
static double start_time = 0;

static void schedule_next_timer(struct timer_data *td) {
  struct timer_info *ti;
  struct timer_info *min_ti = NULL;
  LIST_FOREACH(ti, &td->timers, entries) {
    if (min_ti == NULL || ti->next_invocation < min_ti->next_invocation) {
      min_ti = ti;
    }
  }
  td->current = min_ti;
  td->nc->ev_timer_time = (min_ti != NULL ? min_ti->next_invocation : 0);
}

static void mgos_timer_ev(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  if (ev != MG_EV_TIMER) return;
  timer_callback cb = NULL;
  void *cb_arg = NULL;
  {
    mgos_lock();
    struct timer_data *td = (struct timer_data *) user_data;
    struct timer_info *ti = td->current;
    /* Current can be NULL if it was the first to fire but was cleared. */
    if (ti != NULL) {
      cb = ti->cb;
      cb_arg = ti->cb_arg;
      if (ti->interval_ms >= 0) {
        const double now = mg_time();
        ti->next_invocation += ti->interval_ms / 1000.0;
        /* Polling loop was delayed, bring the invocation time forward to now */
        if (ti->next_invocation < now) ti->next_invocation = now;
        ti = NULL;
      } else {
        LIST_REMOVE(ti, entries);
      }
    }
    schedule_next_timer(td);
    mgos_unlock();
    if (ti != NULL) free(ti);
  }
  if (cb != NULL) cb(cb_arg);
  (void) ev_data;
  (void) nc;
}

mgos_timer_id mgos_set_timer(int msecs, int repeat, timer_callback cb,
                             void *arg) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MGOS_INVALID_TIMER_ID;
  ti->next_invocation = mg_time() + msecs / 1000.0;
  if (repeat) {
    ti->interval_ms = msecs;
  } else {
    ti->interval_ms = -1;
  }
  ti->cb = cb;
  ti->cb_arg = arg;
  {
    mgos_lock();
    LIST_INSERT_HEAD(&s_timer_data->timers, ti, entries);
    schedule_next_timer(s_timer_data);
    mgos_unlock();
  }
  mongoose_schedule_poll(false /* from_isr */);
  return (mgos_timer_id) ti;
}

static void mgos_clear_sw_timer(mgos_timer_id id) {
  struct timer_info *ti = (struct timer_info *) id, *ti2;
  mgos_lock();
  LIST_FOREACH(ti2, &s_timer_data->timers, entries) {
    if (ti2 == ti) break;
  }
  if (ti2 == NULL) return; /* Not a valid timer */
  LIST_REMOVE(ti, entries);
  if (s_timer_data->current == ti) {
    schedule_next_timer(s_timer_data);
    /* Removing a timer can only push back invocation, no need to do a poll. */
  }
  mgos_unlock();
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

#if MGOS_ENABLE_SNTP
static void mgos_time_change_cb(void *arg, double delta) {
  struct timer_data *td = (struct timer_data *) arg;
  mgos_lock();
  struct timer_info *ti;
  LIST_FOREACH(ti, &td->timers, entries) {
    ti->next_invocation += delta;
  }
  start_time += delta;
  mgos_unlock();
}
#endif

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
#if MGOS_ENABLE_SNTP
  mgos_sntp_add_time_change_cb(mgos_time_change_cb, td);
#endif
  return mgos_hw_timers_init();
}

double mgos_uptime(void) {
  return mg_time() - start_time;
}

void mgos_uptime_init(void) {
  start_time = mg_time();
}

int mgos_strftime(char *s, int size, char *fmt, int time) {
  time_t t = (time_t) time;
  struct tm *tmp = localtime(&t);
  return tmp == NULL ? -1 : (int) strftime(s, size, fmt, tmp);
}
