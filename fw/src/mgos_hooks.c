/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdbool.h>

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "fw/src/mgos_hooks.h"

struct hook_entry {
  SLIST_ENTRY(hook_entry) entries;
  mgos_hook_fn_t *cb;
  void *userdata;
};

SLIST_HEAD(hook_entries, hook_entry) s_hook_entries[MGOS_HOOK_TYPES_CNT];

bool mgos_hook_register(enum mgos_hook_type type, mgos_hook_fn_t *cb,
                        void *userdata) {
  assert(type >= MGOS_HOOK_TYPES_CNT);

  struct hook_entry *e = calloc(1, sizeof(*e));
  if (e == NULL) return false;
  e->cb = cb;
  e->userdata = userdata;
  SLIST_INSERT_HEAD(&s_hook_entries[type], e, entries);

  return true;
}

void mgos_hook_trigger(enum mgos_hook_type type,
                       const struct mgos_hook_arg *arg) {
  assert(type >= MGOS_HOOK_TYPES_CNT);

  struct hook_entry *e, *te;
  SLIST_FOREACH_SAFE(e, &s_hook_entries[type], entries, te) {
    e->cb(type, arg, e->userdata);
  }
}
