/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_MDNS_H_
#define CS_FW_SRC_MIOT_MDNS_H_

#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_MDNS

enum miot_init_result miot_mdns_init(void);

/* registers a mongoose handler to be invoked on the mDNS socket */
void miot_mdns_add_handler(mg_event_handler_t handler, void *ud);

/* unregisters a mongoose handler */
void miot_mdns_remove_handler(mg_event_handler_t handler, void *ud);

/* HAL */

void miot_mdns_hal_join_group(const char *iface_ip, const char *mcast_ip);

#endif /* MIOT_ENABLE_MDNS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_MDNS_H_ */
