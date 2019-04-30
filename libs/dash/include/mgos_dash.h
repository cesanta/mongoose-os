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
 * Device management dashboard API.
 */

#ifndef CS_MOS_LIBS_DASH_H_
#define CS_MOS_LIBS_DASH_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Returns true if mDash connection is up, false otherwise. */
bool mgos_dash_is_connected(void);

/*
 * Send an notification RPC to the dashboard that does not require an answer.
 * Example - report statistical data:
 * ```c
 *    mgos_dash_notifyf("Data", "[%d, %d]", value1, value2);
 * ```
 */
void mgos_dash_notifyf(const char *method, const char *json_fmt, ...);

/*
 * Same as `mgos_dash_notifyf()` but accepts stringified data.
 */
void mgos_dash_notify(const char *method, const char *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_DASH_H_ */
