/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_INIT_JS_H_
#define CS_FW_SRC_SJ_INIT_JS_H_

#include "fw/src/sj_init.h"

#if defined(V7_THAW)
#define SJ_PRIVATE /* nothing */
#else
#define SJ_PRIVATE static
#endif /* CS_FW_SRC_SJ_COMMON_H_ */

struct v7;

/*
 * JS initialization is performed in two stages:
 *  - API setup, which just sets up methods and read-only constants.
 *    Nothing that is set up in this phase should be mutable at runtime:
 *    on targets that support freezing these will be kept on immutable storage.
 *  - Runtime init. This where mutable state should be initialized.
 */

enum sj_init_result sj_api_setup(struct v7 *v7);
enum sj_init_result sj_init_js(struct v7 *v7);

enum sj_init_result sj_init_js_all(struct v7 *v7);

#endif /* CS_FW_SRC_SJ_INIT_JS_H_ */
