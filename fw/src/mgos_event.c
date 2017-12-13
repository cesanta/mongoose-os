/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <assert.h>
#include <stdlib.h>

#include "mgos_event.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

struct handler {
  int ev;
  mgos_event_handler_t cb;
  void *userdata;
  SLIST_ENTRY(handler) next;
};

struct event {
  int ev;
  const char *name;
  SLIST_ENTRY(event) next;
};

static SLIST_HEAD(s_events, event) s_events = SLIST_HEAD_INITIALIZER(s_events);
static SLIST_HEAD(s_handlers,
                  handler) s_handlers = SLIST_HEAD_INITIALIZER(s_handlers);

bool mgos_event_register_base(int ev, const char *name) {
  struct event *e;
  SLIST_FOREACH(e, &s_events, next) {
    if (e->ev == ev) {
      LOG(LL_ERROR, ("conflicting event: %s", e->name));
      return false;
    }
  }
  e = calloc(1, sizeof(*e));
  if (e == NULL) return false;
  e->ev = ev;
  e->name = name;
  SLIST_INSERT_HEAD(&s_events, e, next);
  return true;
}

bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata) {
  struct handler *h = calloc(1, sizeof(*h));
  if (h == NULL) return false;
  h->ev = ev;
  h->cb = cb;
  h->userdata = userdata;
  SLIST_INSERT_HEAD(&s_handlers, h, next);
  return true;
}

int mgos_event_trigger(int ev, void *ev_data) {
  struct handler *h, *te;
  int count = 0;
  SLIST_FOREACH_SAFE(h, &s_handlers, next, te) {
    if (h->ev != ev) continue;
    h->cb(ev, ev_data, h->userdata);
    count++;
  }
  if (ev != MGOS_EVENT_LOG) {
    LOG(LL_DEBUG, ("ev %x triggered %d handlers", ev, count));
  }
  return count;
}
