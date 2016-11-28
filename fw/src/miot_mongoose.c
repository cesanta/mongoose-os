/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_mongoose.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "fw/src/miot_hal.h"

#ifndef IRAM
#define IRAM
#endif

struct mg_mgr s_mgr;
static bool s_feed_wdt;
static size_t s_min_free_heap_size;

struct cb_info {
  miot_poll_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(cb_info) poll_cbs;
};
SLIST_HEAD(s_poll_cbs, cb_info) s_poll_cbs = SLIST_HEAD_INITIALIZER(s_poll_cbs);

IRAM struct mg_mgr *miot_get_mgr() {
  return &s_mgr;
}

void mongoose_init(void) {
  mg_mgr_init(&s_mgr, NULL);
}

void mongoose_destroy(void) {
  mg_mgr_free(&s_mgr);
}

int mongoose_poll(int ms) {
  int ret;
  {
    struct cb_info *ci, *cit;
    SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
      ci->cb(ci->cb_arg);
    }
  }

  if (s_feed_wdt) miot_wdt_feed();

  if (mg_next(&s_mgr, NULL) != NULL) {
    mg_mgr_poll(&s_mgr, ms);
    ret = 1;
  } else {
    ret = 0;
  }

  if (s_min_free_heap_size > 0 &&
      miot_get_min_free_heap_size() < s_min_free_heap_size) {
    s_min_free_heap_size = miot_get_min_free_heap_size();
    LOG(LL_INFO, ("New heap free LWM: %d", (int) s_min_free_heap_size));
  }

  return ret;
}

void miot_add_poll_cb(miot_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci = (struct cb_info *) calloc(1, sizeof(*ci));
  ci->cb = cb;
  ci->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&s_poll_cbs, ci, poll_cbs);
}

void miot_remove_poll_cb(miot_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci, *cit;
  SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
    if (ci->cb == cb && ci->cb_arg == cb_arg) {
      SLIST_REMOVE(&s_poll_cbs, ci, cb_info, poll_cbs);
      free(ci);
    }
  }
}

void miot_wdt_set_feed_on_poll(bool enable) {
  s_feed_wdt = (enable != false);
}

void miot_set_enable_min_heap_free_reporting(bool enable) {
  if (!enable && s_min_free_heap_size == 0) return;
  if (enable && s_min_free_heap_size > 0) return;
  s_min_free_heap_size = (enable ? miot_get_min_free_heap_size() : 0);
}
