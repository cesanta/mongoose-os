/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_wifi.h"

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/queue.h"

struct wifi_cb {
  SLIST_ENTRY(wifi_cb) entries;
  sj_wifi_changed_t cb;
  void *arg;
};

SLIST_HEAD(wifi_cbs, wifi_cb) s_wifi_cbs;

void sj_wifi_on_change_cb(enum sj_wifi_status event) {
  switch (event) {
    case SJ_WIFI_DISCONNECTED:
      LOG(LL_INFO, ("Wifi: disconnected"));
      break;
    case SJ_WIFI_CONNECTED:
      LOG(LL_INFO, ("Wifi: connected"));
      break;
    case SJ_WIFI_IP_ACQUIRED: {
      char *ip = sj_wifi_get_sta_ip();
      LOG(LL_INFO, ("WiFi: ready, IP %s", ip));
      free(ip);
      break;
    }
  }

  struct wifi_cb *e, *te;
  SLIST_FOREACH_SAFE(e, &s_wifi_cbs, entries, te) {
    e->cb(event, e->arg);
  }
}

void sj_wifi_add_on_change_cb(sj_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e = calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->cb = cb;
  e->arg = arg;
  SLIST_INSERT_HEAD(&s_wifi_cbs, e, entries);
}

void sj_wifi_remove_on_change_cb(sj_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e;
  SLIST_FOREACH(e, &s_wifi_cbs, entries) {
    if (e->cb == cb && e->arg == arg) {
      SLIST_REMOVE(&s_wifi_cbs, e, wifi_cb, entries);
      return;
    }
  }
}

void sj_wifi_init() {
  sj_wifi_hal_init();
}
