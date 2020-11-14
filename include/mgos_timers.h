/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
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
 *
 * Usage example:
 * ```c
 * #include "mgos_app.h"
 * #include "mgos_system.h"
 * #include "mgos_timers.h"
 *
 * static void my_timer_cb(void *arg) {
 *   bool val = mgos_gpio_toggle(mgos_sys_config_get_pins_led());
 *   LOG(LL_INFO, ("uptime: %.2lf", mgos_uptime()));
 *   (void) arg;
 * }
 *
 * enum mgos_app_init_result mgos_app_init(void) {
 *   mgos_set_timer(1000, MGOS_TIMER_REPEAT, my_timer_cb, NULL);
 *   return MGOS_APP_INIT_SUCCESS;
 * }
 * ```
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HAL */

#define MGOS_INVALID_TIMER_ID 0

/* Timer callback */
typedef void (*timer_callback)(void *param);

/* Timer ID type */
typedef uintptr_t mgos_timer_id;

/* Flags for mgos_set*_timer() */

/*
 * When set, the callback is invoked at the specified interval.
 * Otherwise the call is a one-off.
 */
#define MGOS_TIMER_REPEAT (1 << 0)

/*
 * Flag that makes callback execute immediately.
 * Only makes sense for repeating timers.
 */
#define MGOS_TIMER_RUN_NOW (1 << 1)

/*
 * Setup a timer with `msecs` timeout and `cb` as a callback.
 *
 * `flags` is a bitmask, currently there's only one flag available:
 * `MGOS_TIMER_REPEAT` (see above). `arg` is a parameter to pass to `cb`.
 * Return numeric timer ID.
 *
 * Note that this is a software timer, with fairly low accuracy and high jitter.
 * However, number of software timers is not limited.
 * If you need intervals < 10ms, use mgos_set_hw_timer.
 *
 * Example:
 * ```c
 * static void my_timer_cb(void *arg) {
 *   bool val = mgos_gpio_toggle(mgos_sys_config_get_pins_led());
 *   LOG(LL_INFO, ("uptime: %.2lf", mgos_uptime()));
 *   (void) arg;
 * }
 *
 * enum mgos_app_init_result mgos_app_init(void) {
 *   mgos_set_timer(1000, MGOS_TIMER_REPEAT, my_timer_cb, NULL);
 *   return MGOS_APP_INIT_SUCCESS;
 * }
 * ```
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

struct mgos_timer_info {
  int interval_ms;  // Only valid for repeated timers, 0 for one-shot.
  int msecs_left;   // Until next invocation.
  timer_callback cb;
  void *cb_arg;
};

bool mgos_get_timer_info(mgos_timer_id id, struct mgos_timer_info *info);

#ifdef __cplusplus
}
#endif
