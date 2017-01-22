/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <fw/src/mgos_timers.h>

#include <fw/src/mgos_mongoose.h>
#include <fw/src/mgos_features.h>

static mgos_timer_id s_next_timer_id = 0;

struct timer_info {
  mgos_timer_id id;
  int interval_ms;
  timer_callback cb;
  void *arg;
};

static void mgos_timer_handler(struct mg_connection *c, int ev, void *p) {
  struct timer_info *ti = (struct timer_info *) c->user_data;
  (void) p;
  if (ti == NULL) return;
  switch (ev) {
    case MG_EV_TIMER: {
      if (c->flags & MG_F_CLOSE_IMMEDIATELY) break;
      if (ti->cb != NULL) ti->cb(ti->arg);
      if (ti->interval_ms > 0) {
        c->ev_timer_time = mg_time() + ti->interval_ms / 1000.0;
      } else {
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      free(ti);
      c->user_data = NULL;
      break;
    }
  }
}

static struct mg_connection *mgos_find_timer(mgos_timer_id id) {
  struct mg_connection *c;
  for (c = mg_next(mgos_get_mgr(), NULL); c != NULL;
       c = mg_next(mgos_get_mgr(), c)) {
    if (c->handler == mgos_timer_handler) {
      struct timer_info *ti = (struct timer_info *) c->user_data;
      if (ti != NULL && ti->id == id) return c;
    }
  }
  return NULL;
}

static mgos_timer_id mgos_set_timer_common(struct timer_info *ti, int msecs,
                                           int repeat) {
  struct mg_connection *c;
  struct mg_add_sock_opts opts;
  do {
    ti->id = s_next_timer_id++;
  } while (ti->id == MGOS_INVALID_TIMER_ID || mgos_find_timer(ti->id) != NULL);
  ti->interval_ms = (repeat ? msecs : -1);
  memset(&opts, 0, sizeof(opts));
  opts.user_data = ti;
  c = mg_add_sock_opt(mgos_get_mgr(), INVALID_SOCKET, mgos_timer_handler, opts);
  if (c == NULL) {
    free(ti);
    return 0;
  }
  c->ev_timer_time = mg_time() + (msecs / 1000.0);
  mongoose_schedule_poll();
  return 1;
}

mgos_timer_id mgos_set_timer(int msecs, int repeat, timer_callback cb,
                             void *arg) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MGOS_INVALID_TIMER_ID;
  ti->cb = cb;
  ti->arg = arg;
  if (!mgos_set_timer_common(ti, msecs, repeat)) return MGOS_INVALID_TIMER_ID;
  return ti->id;
}

void mgos_clear_timer(mgos_timer_id id) {
  struct mg_connection *c = mgos_find_timer(id);
  if (c == NULL) return;
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
}
