/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_CONSOLE_JS_H_
#define CS_FW_SRC_MIOT_CONSOLE_JS_H_

#if MIOT_ENABLE_JS
struct v7;
void miot_console_api_setup(struct v7 *v7);
void miot_console_js_init(struct v7 *v7);
#endif /* MIOT_ENABLE_JS */

#endif /* CS_FW_SRC_MIOT_CONSOLE_JS_H_ */
