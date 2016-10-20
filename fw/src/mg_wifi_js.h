/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_WIFI_JS_H_
#define CS_FW_SRC_MG_WIFI_JS_H_

#if MG_ENABLE_WIFI_API
struct v7;
void mg_wifi_api_setup(struct v7 *v7);
void mg_wifi_js_init(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_MG_WIFI_JS_H_ */
