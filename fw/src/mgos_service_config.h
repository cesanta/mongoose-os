/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_SERVICE_CONFIG_H_
#define CS_FW_SRC_MGOS_SERVICE_CONFIG_H_

#if MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE

#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialises mg_rpc handlers for Config commands
 */
enum mgos_init_result mgos_service_config_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE */
#endif /* CS_FW_SRC_MGOS_SERVICE_CONFIG_H_ */
