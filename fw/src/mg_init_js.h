/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_INIT_JS_H_
#define CS_FW_SRC_MG_INIT_JS_H_

#include "fw/src/mg_init.h"

#if defined(V7_THAW)
#define MG_PRIVATE /* nothing */
#else
#define MG_PRIVATE static
#endif

struct v7;

/*
 * JS initialization is performed in two stages:
 *  - API setup, which just sets up methods and read-only constants.
 *    Nothing that is set up in this phase should be mutable at runtime:
 *    on targets that support freezing these will be kept on immutable storage.
 *  - Runtime init. This where mutable state should be initialized.
 */

enum mg_init_result mg_api_setup(struct v7 *v7);
enum mg_init_result mg_init_js(struct v7 *v7);

enum mg_init_result mg_init_js_all(struct v7 *v7);

#endif /* CS_FW_SRC_MG_INIT_JS_H_ */
