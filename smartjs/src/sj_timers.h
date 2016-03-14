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

/* Setup timer with msecs timeout and cb as a callback
 * cb is a v7_own()ed, heap-allocated pointer and must be disowned and freed
 * (after invocation).
 */
void sj_set_timeout(int msecs, v7_val_t *cb);
void sj_set_c_timeout(int msecs, timer_callback cb, void *param);

#endif /* CS_SMARTJS_SRC_SJ_TIMERS_H_ */
