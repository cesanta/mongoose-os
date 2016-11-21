/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_MG_RPC_JS_H_
#define CS_FW_SRC_MIOT_MG_RPC_JS_H_

#if MIOT_ENABLE_RPC && MIOT_ENABLE_JS

#include "v7/v7.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void miot_rpc_api_setup(struct v7 *v7);

void miot_rpc_js_init(struct v7 *v7);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_JS */

#endif /* CS_FW_SRC_MIOT_MG_RPC_JS_H_ */
