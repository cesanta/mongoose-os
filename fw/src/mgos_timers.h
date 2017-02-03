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

#define MGOS_INVALID_TIMER_ID 0

/* Timer callback */
typedef void (*timer_callback)(void *param);

/* Timer ID type */
typedef uint32_t mgos_timer_id;

/*
 * Setup timer with `msecs` timeout and `cb` as a callback.
 *
 * `repeat` set to 1 will repeat a call infinitely, otherwise it's a one-off.
 * `arg` is a parameter to pass to `cb`. Return numeric timer ID.
 */
mgos_timer_id mgos_set_timer(int msecs, int repeat, timer_callback cb,
                             void *arg);

/*
 * Disable timer with a given timer ID.
 */
void mgos_clear_timer(mgos_timer_id id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_TIMERS_H_ */
