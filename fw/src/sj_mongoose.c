/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_mongoose.h"

#include "common/queue.h"

struct mg_mgr sj_mgr;

struct cb_info {
  mg_poll_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(cb_info) poll_cbs;
};
SLIST_HEAD(s_poll_cbs, cb_info) s_poll_cbs = SLIST_HEAD_INITIALIZER(s_poll_cbs);

void mongoose_init(void) {
  mg_mgr_init(&sj_mgr, NULL);
}

void mongoose_destroy(void) {
  mg_mgr_free(&sj_mgr);
}

int mongoose_poll(int ms) {
  {
    struct cb_info *ci, *cit;
    SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
      ci->cb(ci->cb_arg);
    }
  }
  if (mg_next(&sj_mgr, NULL) != NULL) {
    mg_mgr_poll(&sj_mgr, ms);
    return 1;
  } else {
    return 0;
  }
}

void mg_add_poll_cb(mg_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci = (struct cb_info *) calloc(1, sizeof(*ci));
  ci->cb = cb;
  ci->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&s_poll_cbs, ci, poll_cbs);
}

void mg_remove_poll_cb(mg_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci, *cit;
  SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
    if (ci->cb == cb && ci->cb_arg == cb_arg) {
      SLIST_REMOVE(&s_poll_cbs, ci, cb_info, poll_cbs);
      free(ci);
    }
  }
}
