/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_TIMERS_H_
#define CS_FW_SRC_MG_TIMERS_H_

#include <stdint.h>

/* HAL */

/*
 * Setup timer with msecs timeout and cb as a callback.
 * cb is not owned, callee must own it to prevent premature GC.
 */

#define MG_INVALID_TIMER_ID 0

typedef void (*timer_callback)(void *param);
typedef uint32_t mg_timer_id;

mg_timer_id mg_set_c_timer(int msecs, int repeat, timer_callback cb, void *arg);
void mg_clear_timer(mg_timer_id id);

#endif /* CS_FW_SRC_MG_TIMERS_H_ */
