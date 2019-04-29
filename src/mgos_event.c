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

#include <assert.h>
#include <stdlib.h>

#include "mgos_event.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

struct handler {
  int ev;

  mgos_event_handler_t cb;
  void *userdata;

  /*
   * If set, the event handler is intended for all events from
   * `ev & ~0xff` to `ev | 0xff`
   */
  bool group;

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

static bool add_handler(int ev, mgos_event_handler_t cb, void *userdata,
                        bool group) {
  struct handler *h = calloc(1, sizeof(*h));
  if (h == NULL) return false;
  h->ev = ev;
  h->cb = cb;
  h->userdata = userdata;
  h->group = group;

  /* When adding a group handler, make sure `ev` is a base event number */
  if (group) {
    h->ev &= ~0xff;
  }
  SLIST_INSERT_HEAD(&s_handlers, h, next);
  return true;
}

bool mgos_event_add_handler(int ev, mgos_event_handler_t cb, void *userdata) {
  return add_handler(ev, cb, userdata, false);
}

bool mgos_event_add_group_handler(int evgrp, mgos_event_handler_t cb,
                                  void *userdata) {
  return add_handler(evgrp, cb, userdata, true);
}

static bool remove_handler(int ev, mgos_event_handler_t cb, void *userdata,
                           bool group) {
  struct handler *ph = NULL, *h = NULL, *th;
  SLIST_FOREACH_SAFE(h, &s_handlers, next, th) {
    if (h->ev == ev && h->group == group && h->cb == cb &&
        h->userdata == userdata) {
      break;
    }
    ph = h;
  }
  if (h == NULL) return false;
  if (ph == NULL) {
    SLIST_REMOVE_HEAD(&s_handlers, next);
  } else {
    SLIST_REMOVE_AFTER(ph, next);
  }
  free(h);
  return true;
}

bool mgos_event_remove_handler(int ev, mgos_event_handler_t cb,
                               void *userdata) {
  return remove_handler(ev, cb, userdata, false);
}

bool mgos_event_remove_group_handler(int evgrp, mgos_event_handler_t cb,
                                     void *userdata) {
  return remove_handler(evgrp, cb, userdata, true);
}

int mgos_event_trigger(int ev, void *ev_data) {
  struct handler *h, *te;
  int count = 0;
  SLIST_FOREACH_SAFE(h, &s_handlers, next, te) {
    if (h->ev == ev || (h->group && ev >= h->ev && ev <= (h->ev | 0xff))) {
      h->cb(ev, ev_data, h->userdata);
      count++;
    }
  }
  if (ev != MGOS_EVENT_LOG) {
    const uint8_t *u = (uint8_t *) &ev;
    LOG(LL_DEBUG,
        ("ev %c%c%c%hhu triggered %d handlers", u[3], u[2], u[1], u[0], count));
    (void) u;
  }
  return count;
}
