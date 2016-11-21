/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_mdns.h"

#include <stdlib.h>

#include "common/platform.h"
#include "common/cs_dbg.h"
#include "common/queue.h"

#include "fw/src/miot_dns_sd.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

#if MIOT_ENABLE_MDNS

#define MDNS_MCAST_GROUP "224.0.0.251"
#define MDNS_PORT 5353

struct mdns_handler {
  SLIST_ENTRY(mdns_handler) entries;
  mg_event_handler_t handler;
  void *ud;
};

SLIST_HEAD(mdns_handlers, mdns_handler) s_mdns_handlers;

void miot_mdns_add_handler(mg_event_handler_t handler, void *ud) {
  struct mdns_handler *e = calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->handler = handler;
  e->ud = ud;
  SLIST_INSERT_HEAD(&s_mdns_handlers, e, entries);
}

void miot_mdns_remove_handler(mg_event_handler_t handler, void *ud) {
  struct mdns_handler *e;
  SLIST_FOREACH(e, &s_mdns_handlers, entries) {
    if (e->handler == handler && e->ud == ud) {
      SLIST_REMOVE(&s_mdns_handlers, e, mdns_handler, entries);
      return;
    }
  }
}

static void on_wifi_change(enum miot_wifi_status event, void *ud) {
  (void) ud;

  switch (event) {
    case MIOT_WIFI_IP_ACQUIRED: {
      /*
       * multicast is not abstracted properly neither by most SDKs
       * nor my mongoose itself. Each platform will have a hal function
       * that deals with joining a multicast group.
       *
       * TODO(mkm): give mongoose a generic multicast management API.
       */
      char *ip = miot_wifi_get_sta_ip();
      miot_mdns_hal_join_group(ip, MDNS_MCAST_GROUP);
      free(ip);
      break;
    }
    default:
      ;
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

enum miot_init_result miot_mdns_init(void) {
  char listener_spec[128];
  struct mg_mgr *mgr = miot_get_mgr();
  struct mg_connection *lc;

  if (!get_cfg()->dns_sd.enable) return MIOT_INIT_OK;

  snprintf(listener_spec, sizeof(listener_spec), "udp://:%d", MDNS_PORT);
  LOG(LL_INFO, ("Listening on %s", listener_spec));

  lc = mg_bind(mgr, listener_spec, handler);
  if (lc == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return MIOT_INIT_MDNS_FAILED;
  }

  mg_set_protocol_dns(lc);

  /*
   * we had to bind on 0.0.0.0, but now we can store our mdns dest here
   * so we don't need to create a new connection in order to send outbound
   * mcast traffic.
   */
  lc->sa.sin.sin_port = htons(5353);
  inet_aton(MDNS_MCAST_GROUP, &lc->sa.sin.sin_addr);

  /*
   * wait until the STA interface is brought up in order to add it to the
   * multicast group
   */
  miot_wifi_add_on_change_cb(on_wifi_change, NULL);

  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_MDNS */
