/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MDNS_H_
#define CS_FW_SRC_MGOS_MDNS_H_

#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MDNS

enum mgos_init_result mgos_mdns_init(void);

/* registers a mongoose handler to be invoked on the mDNS socket */
void mgos_mdns_add_handler(mg_event_handler_t handler, void *ud);

/* unregisters a mongoose handler */
void mgos_mdns_remove_handler(mg_event_handler_t handler, void *ud);

/* HAL */

void mgos_mdns_hal_join_group(const char *mcast_ip);

#endif /* MGOS_ENABLE_MDNS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MDNS_H_ */
