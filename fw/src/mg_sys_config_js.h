/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_SYS_CONFIG_JS_H_
#define CS_FW_SRC_MG_SYS_CONFIG_JS_H_

#if MG_ENABLE_JS
struct v7;
enum mg_init_result mg_sys_config_js_init(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_MG_SYS_CONFIG_JS_H_ */
