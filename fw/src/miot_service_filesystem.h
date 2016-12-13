/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_SERVICE_FILESYSTEM_H_
#define CS_FW_SRC_MIOT_SERVICE_FILESYSTEM_H_

#if MIOT_ENABLE_RPC && MIOT_ENABLE_FILESYSTEM_SERVICE

#include "fw/src/miot_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialises mg_rpc handlers for /v1/Filesystem commands
 */
enum miot_init_result miot_service_filesystem_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_FILESYSTEM_SERVICE */
#endif /* CS_FW_SRC_MIOT_SERVICE_FILESYSTEM_H_ */
