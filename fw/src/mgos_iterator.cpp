#include <stdlib.h>
#include "mgos.h"
#include "mgos_iterator.h"
#include "mgos_timers.h"

struct mgos_iterator_ctx {
  predicate has_next;
  callable next;
  mgos_timer_id timer_id;
  void *param;
};

struct mgos_iterator_count {
  int current;
  int limit;
  callable_with_index cb;
  mgos_iterator_id iterator;
  void *param;
};

static void mgos_iterator_step(void *arg) {
  struct mgos_iterator_ctx *mi = (struct mgos_iterator_ctx *)arg;
  if (mi->has_next(mi->param)) {
    mi->next(mi->param);
  } else {
    mgos_clear_iterator((mgos_iterator_id)mi);
  }
}

mgos_iterator_id mgos_iterator(int msecs, predicate has_next, timer_callback cb, void *param) {
  struct mgos_iterator_ctx *mi = new struct mgos_iterator_ctx;
  mi->has_next = has_next;
  mi->next = (callable) cb;
  mi->timer_id = mgos_set_timer(msecs, true, mgos_iterator_step, mi);
  mi->param = param;
  return (mgos_iterator_id) mi;
}

bool mgos_iterator_count_has_next(void *arg) {
  struct mgos_iterator_count *ctx = (struct mgos_iterator_count *)arg;
  return ctx->current < ctx->limit;
}

void mgos_iterator_count_next(void *arg) {
  struct mgos_iterator_count *ctx = (struct mgos_iterator_count *)arg;
  ctx->cb(ctx->param, ctx->current);
  ctx->current += 1;
}

void mgos_clear_iterator_count(mgos_iterator_count_id arg) {
  struct mgos_iterator_count *ctx = (struct mgos_iterator_count *)arg;
  mgos_clear_iterator(ctx->iterator);
  delete ctx;
}

mgos_iterator_count_id mgos_iterator_count(int msecs, int limit, callable_with_index cb, void *param) {
  struct mgos_iterator_count *mic = new struct mgos_iterator_count;
  LOG(LL_INFO, ("New count iterator w/ limit %d and param %p", limit, param));
  mic->limit = limit;
  mic->current = 0;
  mic->cb = cb;
  mic->param = param;
  mic->iterator = mgos_iterator(msecs, mgos_iterator_count_has_next, mgos_iterator_count_next, mic);
  return (mgos_iterator_count_id) mic;
}

void mgos_clear_iterator(mgos_iterator_id iterator_id) {
  struct mgos_iterator_ctx *ctx = (struct mgos_iterator_ctx *)iterator_id;
  mgos_clear_timer(ctx->timer_id);
  delete ctx;
}