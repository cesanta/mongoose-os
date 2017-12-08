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

#ifndef CS_FW_SRC_MGOS_MDNS_H_
#define CS_FW_SRC_MGOS_MDNS_H_

#include "mgos_init.h"
#include "mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MDNS

enum mgos_init_result mgos_mdns_init(void);

/* Register a mDNS event handler. */
void mgos_mdns_add_handler(mg_event_handler_t handler, void *ud);

/* Unregister an event handler. */
void mgos_mdns_remove_handler(mg_event_handler_t handler, void *ud);

/* Returns mDNS connection. */
struct mg_connection *mgos_mdns_get_listener(void);

/* Join multicast group. */
void mgos_mdns_hal_join_group(const char *mcast_ip);

/* Leave multicast group. */
void mgos_mdns_hal_leave_group(const char *mcast_ip);

#endif /* MGOS_ENABLE_MDNS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MDNS_H_ */
