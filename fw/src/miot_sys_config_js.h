/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_SYS_CONFIG_JS_H_
#define CS_FW_SRC_MIOT_SYS_CONFIG_JS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_JS
struct v7;
enum miot_init_result miot_sys_config_js_init(struct v7 *v7);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_SYS_CONFIG_JS_H_ */
