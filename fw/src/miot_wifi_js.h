/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_WIFI_JS_H_
#define CS_FW_SRC_MIOT_WIFI_JS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_WIFI_API
struct v7;
void miot_wifi_api_setup(struct v7 *v7);
void miot_wifi_js_init(struct v7 *v7);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_WIFI_JS_H_ */
