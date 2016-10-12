/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CONSOLE_JS_H_
#define CS_FW_SRC_MG_CONSOLE_JS_H_

#ifdef MG_ENABLE_JS
struct v7;
void mg_console_api_setup(struct v7 *v7);
void mg_console_js_init(struct v7 *v7);
#endif /* MG_ENABLE_JS */

#endif /* CS_FW_SRC_MG_CONSOLE_JS_H_ */
