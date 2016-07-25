/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CONSOLE_JS_H_
#define CS_FW_SRC_SJ_CONSOLE_JS_H_

#ifdef SJ_ENABLE_JS
struct v7;
void sj_console_api_setup(struct v7 *v7);
void sj_console_js_init(struct v7 *v7);
#endif /* SJ_ENABLE_JS */

#endif /* CS_FW_SRC_SJ_CONSOLE_JS_H_ */
