/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_wifi.h"

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/queue.h"

struct wifi_cb {
  SLIST_ENTRY(wifi_cb) entries;
  mg_wifi_changed_t cb;
  void *arg;
};

SLIST_HEAD(wifi_cbs, wifi_cb) s_wifi_cbs;

void mg_wifi_on_change_cb(enum mg_wifi_status event) {
  switch (event) {
    case MG_WIFI_DISCONNECTED:
      LOG(LL_INFO, ("Wifi: disconnected"));
      break;
    case MG_WIFI_CONNECTED:
      LOG(LL_INFO, ("Wifi: connected"));
      break;
    case MG_WIFI_IP_ACQUIRED: {
      char *ip = mg_wifi_get_sta_ip();
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

void mg_wifi_add_on_change_cb(mg_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e = calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->cb = cb;
  e->arg = arg;
  SLIST_INSERT_HEAD(&s_wifi_cbs, e, entries);
}

void mg_wifi_remove_on_change_cb(mg_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e;
  SLIST_FOREACH(e, &s_wifi_cbs, entries) {
    if (e->cb == cb && e->arg == arg) {
      SLIST_REMOVE(&s_wifi_cbs, e, wifi_cb, entries);
      return;
    }
  }
}

void mg_wifi_init(void) {
  mg_wifi_hal_init();
}
