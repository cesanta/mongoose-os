/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_TIMERS_JS_H_
#define CS_FW_SRC_SJ_TIMERS_JS_H_

#ifdef SJ_ENABLE_JS

#include <stdint.h>

#include "fw/src/sj_timers.h"
#include "v7/v7.h"

struct v7;
void sj_timers_api_setup(struct v7 *v7);
sj_timer_id sj_set_js_timer(int msecs, int repeat, struct v7 *v7, v7_val_t cb);

#endif /* SJ_ENABLE_JS */

#endif /* CS_FW_SRC_SJ_TIMERS_JS_H_ */
