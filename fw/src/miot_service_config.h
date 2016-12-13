/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_SERVICE_CONFIG_H_
#define CS_FW_SRC_MIOT_SERVICE_CONFIG_H_

#if MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE

#include "fw/src/miot_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialises mg_rpc handlers for /v1/Config commands
 */
enum miot_init_result miot_service_config_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE */
#endif /* CS_FW_SRC_MIOT_SERVICE_CONFIG_H_ */
