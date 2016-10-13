/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_TIMERS_JS_H_
#define CS_FW_SRC_MG_TIMERS_JS_H_

#include "fw/src/mg_features.h"

#if MG_ENABLE_JS

#include <stdint.h>

#include "fw/src/mg_timers.h"
#include "v7/v7.h"

struct v7;
void mg_timers_api_setup(struct v7 *v7);
mg_timer_id mg_set_js_timer(int msecs, int repeat, struct v7 *v7, v7_val_t cb);

#endif /* MG_ENABLE_JS */

#endif /* CS_FW_SRC_MG_TIMERS_JS_H_ */
