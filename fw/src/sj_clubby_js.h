/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CLUBBY_JS_H_
#define CS_FW_SRC_SJ_CLUBBY_JS_H_

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_JS)

#include "v7/v7.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void sj_clubby_api_setup(struct v7 *v7);

void sj_clubby_js_init(struct v7 *v7);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_JS) */

#endif /* CS_FW_SRC_SJ_CLUBBY_JS_H_ */
