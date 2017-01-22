/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_APP_H_
#define CS_FW_SRC_MGOS_APP_H_

#include "fw/src/mgos_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_app_init_result {
  MGOS_APP_INIT_SUCCESS = 0,
  MGOS_APP_INIT_ERROR = -2,
};

/* User app init functions, C and JS respectively.
 * A weak stub is provided in mgos_app_init.c, which can be overridden. */
enum mgos_app_init_result mgos_app_init(void);

/*
 * An early init hook, for apps that want to take control early
 * in the init process. How early? very, very early. If the platform
 * uses RTOS, it is not running yet. Dynamic memory allocation is not
 * safe. Networking is not running. The only safe thing to do is to
 * communicate to mg_app_init something via global variables or shut
 * down the processor and go (back) to sleep.
 */
void mgos_app_preinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_APP_H_ */
