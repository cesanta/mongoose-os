/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_TIMERS_H_
#define CS_FW_SRC_MGOS_TIMERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* HAL */

/*
 * Setup timer with msecs timeout and cb as a callback.
 * cb is not owned, callee must own it to prevent premature GC.
 */

#define MGOS_INVALID_TIMER_ID 0

typedef void (*timer_callback)(void *param);
typedef uint32_t mgos_timer_id;

mgos_timer_id mgos_set_timer(int msecs, int repeat, timer_callback cb,
                             void *arg);
void mgos_clear_timer(mgos_timer_id id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_TIMERS_H_ */
