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
 * This library implements cron core functionality.
 */

#ifndef CS_MOS_LIBS_CRON_SRC_MGOS_CRON_H_
#define CS_MOS_LIBS_CRON_SRC_MGOS_CRON_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MGOS_INVALID_CRON_ID (0)

/* Opaque type for the cron ID */
typedef uintptr_t mgos_cron_id_t;

/*
 * Cron callback signature; `user_data` is a pointer given to
 * `mgos_cron_add()`, and `id` is the id of the corresponding cron job.
 */
typedef void (*mgos_cron_callback_t)(void *user_data, mgos_cron_id_t id);

/*
 * Adds cron entry with the expression `expr` (a null-terminated string, should
 * be no longer that 256 bytes) and `cb` as a callback.
 * `user_data` is an arbitrary pointer which will be passed to `cb`.
 * Returns cron ID.
 */
mgos_cron_id_t mgos_cron_add(const char *expr, mgos_cron_callback_t cb,
                             void *user_data);

/*
 * Calculate the next fire date after the specified date.
 */
time_t mgos_cron_get_next_invocation(mgos_cron_id_t id, time_t date);

/*
 * Returns whether the given string is a valid cron expression or not. In case
 * of an error, if `perr` is not NULL, `*perr` is set to an error message; it
 * should NOT be freed by the caller.
 */
bool mgos_cron_is_expr_valid(const char *expr, const char **perr);

/*
 * Returns user data pointer associated with the given cron job id.
 */
void *mgos_cron_get_user_data(mgos_cron_id_t id);

/*
 * Removes cron entry with a given cron ID.
 */
void mgos_cron_remove(mgos_cron_id_t id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_CRON_SRC_MGOS_CRON_H_ */
