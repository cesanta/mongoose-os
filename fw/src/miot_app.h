/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_APP_H_
#define CS_FW_SRC_MIOT_APP_H_

#include "fw/src/miot_features.h"

enum miot_app_init_result {
  MIOT_APP_INIT_SUCCESS = 0,
  MIOT_APP_INIT_ERROR = -2,
};

/* User app init functions, C and JS respectively.
 * A weak stub is provided in miot_app_init.c, which can be overridden. */
enum miot_app_init_result miot_app_init(void);
#if MIOT_ENABLE_JS
struct v7;
enum miot_app_init_result miot_app_init_js(struct v7 *v7);
#endif

#endif /* CS_FW_SRC_MIOT_APP_H_ */
