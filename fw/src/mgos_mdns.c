/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_mdns.h"

#include <stdlib.h>

#include "common/platform.h"
#include "common/cs_dbg.h"
#include "common/queue.h"

#include "fw/src/mgos_dns_sd.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"

#if MGOS_ENABLE_MDNS

#define MDNS_MCAST_GROUP "224.0.0.251"
#define MDNS_PORT 5353

struct mdns_handler {
  SLIST_ENTRY(mdns_handler) entries;
  mg_event_handler_t handler;
  void *ud;
};

SLIST_HEAD(mdns_handlers, mdns_handler) s_mdns_handlers;

void mgos_mdns_add_handler(mg_event_handler_t handler, void *ud) {
  struct mdns_handler *e = calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->handler = handler;
  e->ud = ud;
  SLIST_INSERT_HEAD(&s_mdns_handlers, e, entries);
}

void mgos_mdns_remove_handler(mg_event_handler_t handler, void *ud) {
  struct mdns_handler *e;
  SLIST_FOREACH(e, &s_mdns_handlers, entries) {
    if (e->handler == handler && e->ud == ud) {
      SLIST_REMOVE(&s_mdns_handlers, e, mdns_handler, entries);
      return;
    }
  }
}

static void handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct mdns_handler *e;
  (void) ev_data;
  SLIST_FOREACH(e, &s_mdns_handlers, entries) {
    void *old_ud = nc->user_data;
    nc->user_data = e->ud;
    e->handler(nc, ev, ev_data);
    nc->user_data = old_ud;
  }
}

enum mgos_init_result mgos_mdns_init(void) {
  char listener_spec[128];
  struct mg_mgr *mgr = mgos_get_mgr();
  struct mg_connection *lc;

  if (!get_cfg()->dns_sd.enable) return MGOS_INIT_OK;

  snprintf(listener_spec, sizeof(listener_spec), "udp://:%d", MDNS_PORT);
  LOG(LL_INFO, ("Listening on %s", listener_spec));

  lc = mg_bind(mgr, listener_spec, handler);
  if (lc == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return MGOS_INIT_MDNS_FAILED;
  }

  mg_set_protocol_dns(lc);

  /*
   * we had to bind on 0.0.0.0, but now we can store our mdns dest here
   * so we don't need to create a new connection in order to send outbound
   * mcast traffic.
   */
  lc->sa.sin.sin_port = htons(5353);
  inet_aton(MDNS_MCAST_GROUP, &lc->sa.sin.sin_addr);

  mgos_mdns_hal_join_group(MDNS_MCAST_GROUP);

  return MGOS_INIT_OK;
}

#endif /* MGOS_ENABLE_MDNS */
