/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_MG_RPC_H_
#define CS_FW_SRC_MIOT_MG_RPC_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_RPC

#include "common/mg_rpc/mg_rpc.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum miot_init_result miot_rpc_init(void);
struct mg_rpc *miot_rpc_get_global(void);
struct mg_rpc_cfg *miot_rpc_cfg_from_sys(const struct sys_config *scfg);
struct mg_rpc_channel_ws_out_cfg *miot_rpc_channel_ws_out_cfg_from_sys(
    const struct sys_config *scfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_RPC */
#endif /* CS_FW_SRC_MIOT_MG_RPC_H_ */
