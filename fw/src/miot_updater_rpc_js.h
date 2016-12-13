/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_SRC_MIOT_UPDATER_MG_RPC_JS_H_
#define CS_FW_SRC_MIOT_UPDATER_MG_RPC_JS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_JS && MIOT_ENABLE_RPC
struct v7;
void mg_updater_mg_rpc_js_init(struct v7 *v7);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_MG_RPC_JS_H_ */
