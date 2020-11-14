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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Event data for the `MGOS_EVENT_TIME_CHANGED` event, see
 * `mgos_event_add_handler()`.
 */
struct mgos_time_changed_arg {
  double delta;
};

struct timezone;

/* Get number of seconds since last reboot */
double mgos_uptime(void);

/* Get number of microseconds since last reboot */
int64_t mgos_uptime_micros(void);

/* Get wall time in microseconds. */
int64_t mgos_time_micros(void);

/*
 * Format `time` according to a `strftime()`-conformant format.
 * Write the result into the `s,size` buffer. Return resulting string length.
 */
int mgos_strftime(char *s, int size, const char *fmt, int time);

/*
 * Like standard `settimeofday()`, but uses `double` seconds value instead of
 * `struct timeval *tv`. If time was changed successfully, emits an event
 * `MGOS_EVENT_TIME_CHANGED`.
 */
int mgos_settimeofday(double time, struct timezone *tz);

#ifdef __cplusplus
}
#endif
