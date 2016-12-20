/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <fw/src/miot_timers.h>

#include <fw/src/miot_mongoose.h>
#include <fw/src/miot_features.h>

#if MIOT_ENABLE_JS
#include <v7/v7.h>
#include <fw/src/miot_v7_ext.h>
#endif

static miot_timer_id s_next_timer_id = 0;

struct timer_info {
  miot_timer_id id;
  int interval_ms;
  timer_callback cb;
  void *arg;
#if MIOT_ENABLE_JS
  struct v7 *v7;
  v7_val_t js_cb;
#endif
};

static void miot_timer_handler(struct mg_connection *c, int ev, void *p) {
  struct timer_info *ti = (struct timer_info *) c->user_data;
  (void) p;
  if (ti == NULL) return;
  switch (ev) {
    case MG_EV_TIMER: {
      if (c->flags & MG_F_CLOSE_IMMEDIATELY) break;
      if (ti->cb != NULL) ti->cb(ti->arg);
#if MIOT_ENABLE_JS
/* TODO(dfrank) reimplement on top of MJS */
#if 0
      if (ti->v7 != NULL) miot_invoke_js_cb0(ti->v7, ti->js_cb);
#endif
#endif
      if (ti->interval_ms > 0) {
        c->ev_timer_time = mg_time() + ti->interval_ms / 1000.0;
      } else {
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
#if MIOT_ENABLE_JS
      if (ti->v7 != NULL) v7_disown(ti->v7, &ti->js_cb);
#endif
      free(ti);
      c->user_data = NULL;
      break;
    }
  }
}

static struct mg_connection *miot_find_timer(miot_timer_id id) {
  struct mg_connection *c;
  for (c = mg_next(miot_get_mgr(), NULL); c != NULL;
       c = mg_next(miot_get_mgr(), c)) {
    if (c->handler == miot_timer_handler) {
      struct timer_info *ti = (struct timer_info *) c->user_data;
      if (ti != NULL && ti->id == id) return c;
    }
  }
  return NULL;
}

static miot_timer_id miot_set_timer_common(struct timer_info *ti, int msecs,
                                           int repeat) {
  struct mg_connection *c;
  struct mg_add_sock_opts opts;
  do {
    ti->id = s_next_timer_id++;
  } while (ti->id == MIOT_INVALID_TIMER_ID || miot_find_timer(ti->id) != NULL);
  ti->interval_ms = (repeat ? msecs : -1);
  memset(&opts, 0, sizeof(opts));
  opts.user_data = ti;
  c = mg_add_sock_opt(miot_get_mgr(), INVALID_SOCKET, miot_timer_handler, opts);
  if (c == NULL) {
    free(ti);
    return 0;
  }
  c->ev_timer_time = mg_time() + (msecs / 1000.0);
  mongoose_schedule_poll();
  return 1;
}

#if MIOT_ENABLE_JS
miot_timer_id miot_set_js_timer(int msecs, int repeat, struct v7 *v7,
                                v7_val_t cb) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MIOT_INVALID_TIMER_ID;
  ti->v7 = v7;
  ti->js_cb = cb;
  if (!miot_set_timer_common(ti, msecs, repeat)) return MIOT_INVALID_TIMER_ID;
  v7_own(v7, &ti->js_cb);
  return ti->id;
}
#endif

miot_timer_id miot_set_timer(int msecs, int repeat, timer_callback cb,
                             void *arg) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return MIOT_INVALID_TIMER_ID;
  ti->cb = cb;
  ti->arg = arg;
  if (!miot_set_timer_common(ti, msecs, repeat)) return MIOT_INVALID_TIMER_ID;
  return ti->id;
}

void miot_clear_timer(miot_timer_id id) {
  struct mg_connection *c = miot_find_timer(id);
  if (c == NULL) return;
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
}
