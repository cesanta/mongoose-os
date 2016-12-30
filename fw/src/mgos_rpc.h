/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MG_RPC_H_
#define CS_FW_SRC_MGOS_MG_RPC_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_RPC

#include "common/mg_rpc/mg_rpc.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result mgos_rpc_init(void);
struct mg_rpc *mgos_rpc_get_global(void);
struct mg_rpc_cfg *mgos_rpc_cfg_from_sys(const struct sys_config *scfg);
struct mg_rpc_channel_ws_out_cfg *mgos_rpc_channel_ws_out_cfg_from_sys(
    const struct sys_config *scfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_RPC */
#endif /* CS_FW_SRC_MGOS_MG_RPC_H_ */
