/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_NET_HAL_H_
#define CS_FW_SRC_MGOS_NET_HAL_H_

#include "mgos_net.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Invoke this when interface connection state changes. */
void mgos_net_dev_event_cb(enum mgos_net_if_type if_type, int if_instance,
                           enum mgos_net_event ev);

#ifdef MGOS_HAVE_ETHERNET
bool mgos_eth_dev_get_ip_info(int if_instance,
                              struct mgos_net_ip_info *ip_info);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_NET_HAL_H_ */
