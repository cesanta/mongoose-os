/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_APP_H_
#define CS_FW_SRC_MIOT_APP_H_

#include "fw/src/miot_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

/*
 * An early init hook, for apps that want to take control early
 * in the init process. How early? very, very early. If the platform
 * uses RTOS, it is not running yet. Dynamic memory allocation is not
 * safe. Networking is not running. The only safe thing to do is to
 * communicate to mg_app_init something via global variables or shut
 * down the processor and go (back) to sleep.
 */
void miot_app_preinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_APP_H_ */
