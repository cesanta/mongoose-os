/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_MDNS_H_
#define CS_FW_SRC_MG_MDNS_H_

#include "fw/src/mg_init.h"
#include "fw/src/mg_mongoose.h"

#ifdef MG_ENABLE_MDNS

enum mg_init_result mg_mdns_init(void);

/* registers a mongoose handler to be invoked on the mDNS socket */
void mg_mdns_add_handler(mg_event_handler_t handler, void *ud);

/* unregisters a mongoose handler */
void mg_mdns_remove_handler(mg_event_handler_t handler, void *ud);

/* HAL */

void mg_mdns_hal_join_group(const char *iface_ip, const char *mcast_ip);

#endif /* MG_ENABLE_MDNS */

#endif /* CS_FW_SRC_MG_MDNS_H_ */
