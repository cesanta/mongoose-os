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

#include "mgos_mdns_internal.h"

#include <stdlib.h>

#include "common/platform.h"
#include "common/cs_dbg.h"
#include "common/queue.h"

#include "mgos_mongoose.h"
#include "mgos_sys_config.h"

#define MDNS_MCAST_GROUP "224.0.0.251"
#define MDNS_PORT 5353

struct mdns_handler {
  SLIST_ENTRY(mdns_handler) entries;
  mg_event_handler_t handler;
  void *ud;
};

static struct mg_connection *s_listening_mdns_conn;

SLIST_HEAD(mdns_handlers, mdns_handler) s_mdns_handlers;

struct mg_connection *mgos_mdns_get_listener(void) {
  return s_listening_mdns_conn;
}

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

static void handler(struct mg_connection *nc, int ev, void *ev_data,
                    void *user_data) {
  struct mdns_handler *e;
  (void) ev_data;
  (void) user_data;
  SLIST_FOREACH(e, &s_mdns_handlers, entries) {
    e->handler(nc, ev, ev_data, e->ud);
  }
  /* On close, invalidate listener - reconnect */
  if (ev == MG_EV_CLOSE && nc == s_listening_mdns_conn) {
    s_listening_mdns_conn = NULL;
    mgos_mdns_init();
  }
}

bool mgos_mdns_init(void) {
  char listener_spec[128];
  struct mg_mgr *mgr = mgos_get_mgr();

  snprintf(listener_spec, sizeof(listener_spec), "udp://:%d", MDNS_PORT);
  LOG(LL_INFO, ("Listening on %s", listener_spec));

  s_listening_mdns_conn = mg_bind(mgr, listener_spec, handler, NULL);
  if (s_listening_mdns_conn == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return false;
  }

  mg_set_protocol_dns(s_listening_mdns_conn);

  /*
   * we had to bind on 0.0.0.0, but now we can store our mdns dest here
   * so we don't need to create a new connection in order to send outbound
   * mcast traffic.
   */
  s_listening_mdns_conn->sa.sin.sin_port = htons(5353);
  inet_aton(MDNS_MCAST_GROUP, &s_listening_mdns_conn->sa.sin.sin_addr);

  mgos_mdns_hal_join_group(MDNS_MCAST_GROUP);

  return true;
}
