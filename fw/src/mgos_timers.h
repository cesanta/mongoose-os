/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * View this file on GitHub:
 * [mgos_timers.h](https://github.com/cesanta/mongoose-os/blob/master/mgos_timers.h)
 */

#ifndef CS_FW_SRC_MGOS_TIMERS_H_
#define CS_FW_SRC_MGOS_TIMERS_H_

#include <stdint.h>

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* HAL */

#define MGOS_INVALID_TIMER_ID 0

/* Timer callback */
typedef void (*timer_callback)(void *param);

/* Timer ID type */
typedef uintptr_t mgos_timer_id;

#define MGOS_TIMER_REPEAT 1

/*
 * Setup a timer with `msecs` timeout and `cb` as a callback.
 *
 * `flags` set to 1 will repeat a call infinitely, otherwise it's a one-off.
 * `arg` is a parameter to pass to `cb`. Return numeric timer ID.
 *
 * Note that this is a software timer, with fairly low accuracy and high jitter.
 * However, number of software timers is not limited.
 * If you need intervals < 10ms, use mgos_set_hw_timer.
 */
mgos_timer_id mgos_set_timer(int msecs, int flags, timer_callback cb,
                             void *cb_arg);

/*
 * Setup a hardware timer with `usecs` timeout and `cb` as a callback.
 *
 * This is similar to mgos_set_timer, but can be used for shorter intervals
 * (note that time unit is microseconds).
 *
 * Number of hardware timers is limited (ESP8266: 1, ESP32: 4, CC32xx: 4).
 *
 * Callback is executed in ISR context, with all the implications of that.
 */
mgos_timer_id mgos_set_hw_timer(int usecs, int flags, timer_callback cb,
                                void *cb_arg);

/*
 * Disable timer with a given timer ID.
 */
void mgos_clear_timer(mgos_timer_id id);

enum mgos_init_result mgos_timers_init(void);

/* Get number of seconds since last reboot */
double mgos_uptime(void);
void mgos_uptime_init(void);

int mgos_strftime(char *s, int size, char *fmt, int time);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_TIMERS_H_ */
