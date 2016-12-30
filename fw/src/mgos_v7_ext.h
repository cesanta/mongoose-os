/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_V7_EXT_H_
#define CS_FW_SRC_MGOS_V7_EXT_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_JS

#include "v7/v7.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct v7;

/* Initialize objects and functions provided by v7_ext, incl. Sys.* */
void mgos_v7_ext_api_setup(struct v7 *v7);

/* Initialize `Sys.*` */
void mgos_sys_js_init(struct v7 *v7);

/* Prints an exception to stdout or stderr depending on debug mode */
void mgos_print_exception(struct v7 *v7, v7_val_t exc, const char *msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_JS */

#endif /* CS_FW_SRC_MGOS_V7_EXT_H_ */
