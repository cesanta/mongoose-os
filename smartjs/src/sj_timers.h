/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_TIMERS_H_
#define CS_SMARTJS_SRC_SJ_TIMERS_H_

#include "v7/v7.h"

typedef void (*timer_callback)(void *param);
void sj_timers_api_setup(struct v7 *v7);

/* HAL */

/* Setup timer with msecs timeout and cb as a callback.
 * cb is not owned, callee must own it to prevent premature GC.
 */

#define SJ_INVALID_TIMER_ID 0
typedef uint32_t sj_timer_id;
sj_timer_id sj_set_js_timer(int msecs, int repeat, struct v7 *v7, v7_val_t cb);
sj_timer_id sj_set_c_timer(int msecs, int repeat, timer_callback cb, void *arg);
void sj_clear_timer(sj_timer_id id);

#endif /* CS_SMARTJS_SRC_SJ_TIMERS_H_ */
