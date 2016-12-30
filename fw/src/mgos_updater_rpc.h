/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_
#define CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_

#include <inttypes.h>
#include "common/mg_str.h"
#include "fw/src/mgos_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_UPDATER_RPC && MGOS_ENABLE_RPC
void mgos_updater_rpc_init(void);
void mgos_updater_rpc_finish(int error_code, int64_t id,
                             const struct mg_str src);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_ */
