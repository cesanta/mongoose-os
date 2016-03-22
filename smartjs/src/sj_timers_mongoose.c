#include <v7/v7.h>
#include <smartjs/src/sj_timers.h>

#include <smartjs/src/sj_mongoose.h>
#include <smartjs/src/sj_v7_ext.h>

static sj_timer_id s_next_timer_id = 0;

struct timer_info {
  sj_timer_id id;
  int interval_ms;
  timer_callback cb;
  void *arg;
  struct v7 *v7;
  v7_val_t js_cb;
};

static void sj_timer_handler(struct mg_connection *c, int ev, void *p) {
  struct timer_info *ti = (struct timer_info *) c->user_data;
  (void) p;
  if (ti == NULL) return;
  switch (ev) {
    case MG_EV_TIMER: {
      if (c->flags & MG_F_CLOSE_IMMEDIATELY) break;
      if (ti->v7 != NULL) sj_invoke_cb0(ti->v7, ti->js_cb);
      if (ti->cb != NULL) ti->cb(ti->arg);
      if (ti->interval_ms > 0) {
        c->ev_timer_time = mg_time() + ti->interval_ms / 1000.0;
        LOG(LL_INFO, ("next timer at %d", (int) (c->ev_timer_time * 1000)));
      } else {
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ti->v7 != NULL) v7_disown(ti->v7, &ti->js_cb);
      free(ti);
      c->user_data = NULL;
      break;
    }
  }
}

static struct mg_connection *sj_find_timer(sj_timer_id id) {
  struct mg_connection *c;
  for (c = mg_next(&sj_mgr, NULL); c != NULL; c = mg_next(&sj_mgr, c)) {
    if (c->handler == sj_timer_handler) {
      struct timer_info *ti = (struct timer_info *) c->user_data;
      if (ti != NULL && ti->id == id) return c;
    }
  }
  return NULL;
}

sj_timer_id sj_set_timer(struct timer_info *ti, int msecs, int repeat) {
  struct mg_connection *c;
  struct mg_add_sock_opts opts;
  do {
    ti->id = s_next_timer_id++;
  } while (ti->id == SJ_INVALID_TIMER_ID || sj_find_timer(ti->id) != NULL);
  ti->interval_ms = (repeat ? msecs : -1);
  memset(&opts, 0, sizeof(opts));
  opts.user_data = ti;
  c = mg_add_sock_opt(&sj_mgr, INVALID_SOCKET, sj_timer_handler, opts);
  if (c == NULL) {
    free(ti);
    return 0;
  }
  c->ev_timer_time = mg_time() + (msecs / 1000.0);
  mongoose_schedule_poll();
  return 1;
}

sj_timer_id sj_set_js_timer(int msecs, int repeat, struct v7 *v7, v7_val_t cb) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return SJ_INVALID_TIMER_ID;
  ti->v7 = v7;
  ti->js_cb = cb;
  if (!sj_set_timer(ti, msecs, repeat)) return SJ_INVALID_TIMER_ID;
  v7_own(v7, &ti->js_cb);
  return ti->id;
}

sj_timer_id sj_set_c_timer(int msecs, int repeat, timer_callback cb,
                           void *arg) {
  struct timer_info *ti = (struct timer_info *) calloc(1, sizeof(*ti));
  if (ti == NULL) return SJ_INVALID_TIMER_ID;
  ti->cb = cb;
  ti->arg = arg;
  if (!sj_set_timer(ti, msecs, repeat)) return SJ_INVALID_TIMER_ID;
  return ti->id;
}

void sj_clear_timer(sj_timer_id id) {
  struct mg_connection *c = sj_find_timer(id);
  if (c == NULL) return;
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
}
