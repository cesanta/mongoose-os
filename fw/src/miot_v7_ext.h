/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_V7_EXT_H_
#define CS_FW_SRC_MIOT_V7_EXT_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_JS

#include "v7/v7.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct v7;

/* Initialize objects and functions provided by v7_ext, incl. Sys.* */
void miot_v7_ext_api_setup(struct v7 *v7);

/* Initialize `Sys.*` */
void miot_sys_js_init(struct v7 *v7);

/* Prints an exception to stdout or stderr depending on debug mode */
void miot_print_exception(struct v7 *v7, v7_val_t exc, const char *msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_JS */

#endif /* CS_FW_SRC_MIOT_V7_EXT_H_ */
