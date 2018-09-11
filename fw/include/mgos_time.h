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
 * # Time
 *
 * Various time API.
 */

#ifndef CS_FW_INCLUDE_MGOS_TIME_H_
#define CS_FW_INCLUDE_MGOS_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

/*
 * Format `time` according to a `strftime()`-conformant format.
 * Write the result into the `s,size` buffer. Return resulting string length.
 */
int mgos_strftime(char *s, int size, char *fmt, int time);

/*
 * Like standard `settimeofday()`, but uses `double` seconds value instead of
 * `struct timeval *tv`. If time was changed successfully, emits an event
 * `MGOS_EVENT_TIME_CHANGED`.
 */
int mgos_settimeofday(double time, struct timezone *tz);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_TIME_H_ */
