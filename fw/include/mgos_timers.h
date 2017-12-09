/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Timers API.
 *
 * Mongoose OS supports two types of timers: software timers and hardware
 * timers.
 *
 * - Software timers. Implemented as Mongoose library events, in software.
 *   Timer callback is called in a Mongoose task context. Frequency is
 *   specified in milliseconds. Number of software timers is not limited.
 *   Timer intervals cannot be short - limited by the underlying
 *   task scheduling. For example, if you want a very frequent sensor reading,
 *   like thousand readings a second, use hardware timer instead.
 *   Both C and JavaScript API is available.
 * - Hardware timers. Implemented in hardware. Timer callback is executed in
 *   the ISR context, therefore it can do a limited set of actions.
 *   Number of hardware timers is limied: (ESP8266: 1, ESP32: 4, CC32xx: 4).
 *   Frequency is specified in microseconds. Only C API is present, because
 *   calling to JS requires switching to Mongoose task context.
 */

#ifndef CS_FW_INCLUDE_MGOS_TIMERS_H_
#define CS_FW_INCLUDE_MGOS_TIMERS_H_

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

/* Flag for mgos_set*_timer() */
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

/* Initialize uptime */
void mgos_uptime_init(void);

/*
 * Format `time` according to a `strftime()`-conformant format.
 * Write the result into the `s,size` buffer. Return resulting string length.
 */
int mgos_strftime(char *s, int size, char *fmt, int time);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_TIMERS_H_ */
