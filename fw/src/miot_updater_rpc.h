/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_MIOT_UPDATER_MG_RPC_H_
#define CS_FW_SRC_MIOT_UPDATER_MG_RPC_H_

#include <inttypes.h>
#include "common/mg_str.h"
#include "fw/src/miot_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_UPDATER_RPC && MIOT_ENABLE_RPC
void miot_updater_rpc_init(void);
void miot_updater_rpc_finish(int error_code, int64_t id,
                             const struct mg_str src);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_MG_RPC_H_ */
