/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Multicast DNS API.
 *
 * See https://en.wikipedia.org/wiki/Multicast_DNS for for information
 * about the multicast DNS.
 */

#ifndef CS_FW_INCLUDE_MGOS_MDNS_H_
#define CS_FW_INCLUDE_MGOS_MDNS_H_

#include "mgos_features.h"
#include "mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MDNS

/*
 * Register a mDNS event handler `handler` with the arbitrary userdata `ud`.
 *
 * Example:
 *
 * ```c
 * static void handler(struct mg_connection *nc, int ev, void *ev_data,
 *                     void *user_data) {
 *   if (ev == MG_DNS_MESSAGE) {
 *     struct mg_dns_message *msg = (struct mg_dns_message *) ev_data;
 *     char *peer = inet_ntoa(nc->sa.sin.sin_addr);
 *     LOG(LL_DEBUG, ("---- DNS packet from %s (%d questions, %d answers)",
 *peer,
 *           msg->num_questions, msg->num_answers));
 *   }
 *
 *   (void) user_data;
 * }
 *
 * ....
 *
 * // Somewhere else:
 * mgos_mdns_add_handler(handler, NULL)
 * ```
 */
void mgos_mdns_add_handler(mg_event_handler_t handler, void *ud);

/*
 * Unregister a previously registered event handler with the given userdata
 * `ud`.
 */
void mgos_mdns_remove_handler(mg_event_handler_t handler, void *ud);

/* Return global mDNS connection. */
struct mg_connection *mgos_mdns_get_listener(void);

#endif /* MGOS_ENABLE_MDNS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_MDNS_H_ */
