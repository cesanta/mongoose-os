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
 * Crontab wraps [cron core](https://github.com/cesanta/mongoose-os/libs/cron) and
 * maintains a persisted set of cron jobs. Crontab file is simply a JSON file
 * (actually managed by [jstore](https://github.com/cesanta/mongoose-os/libs/jstore))
 * which looks like this:
 *
 * ```json
 * {"items":[
 *   ["1", {
 *     "at": "0 0 7 * * MON-FRI",
 *     "enable": true,
 *     "action": "foo",
 *     "payload": {"a": 1, "b": 2}
 *   }],
 *   ["2", {
 *     "at": "0 30 23 30 * *",
 *     "enable": true,
 *     "action": "bar"
 *   }]
 * ]}
 * ```
 *
 * This file is maintained by a set of API functions (see below).
 *
 * Obviously, crontab file contains a set of cron jobs, each of which consists,
 * at least, of the cron expression like `0 0 7 * * MON-FRI` (refer to the
 * [cron core](https://github.com/cesanta/mongoose-os/libs/cron) for the expression
 * syntax docs) and an action to be taken. Action is just a string, in the
 * example above there are two actions: `foo` and `bar`. Additionally, there
 * can be a `payload`, which is an arbitrary JSON. Payload is just a set of
 * parameters for the action.
 *
 * Obviously, there should be a mapping between those string actions and the
 * corresponding functions to be called; this is what
 * `mgos_crontab_register_handler()` is for.
 *
 * Example:
 *
 * ```c
 * static void my_foo_cb(struct mg_str action,
 *                       struct mg_str payload, void *userdata) {
 *   LOG(LL_INFO, ("Crontab foo job fired! Payload: %.*s", payload.len,
 *payload.p));
 *   (void) action;
 *   (void) userdata;
 * }
 *
 * // Somewhere else:
 * mgos_crontab_register_handler("foo", my_foo_cb, NULL);
 * ```
 *
 * The code above maps action `foo` in the JSON to the callback `my_foo_cb`.
 */

#ifndef CS_MOS_LIBS_CRONTAB_SRC_CRONTAB_H_
#define CS_MOS_LIBS_CRONTAB_SRC_CRONTAB_H_

#include <stdbool.h>

#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * An opaque type for the crontab job ID.
 */
typedef intptr_t mgos_crontab_job_id_t;

/*
 * Invalid value for the crontab job id.
 */
#define MGOS_CRONTAB_INVALID_JOB_ID ((mgos_crontab_job_id_t) 0)

/*
 * Callback for `mgos_crontab_iterate()`; all string data is invalidated when
 * the callback returns.
 */
typedef void (*mgos_crontab_iterate_cb)(mgos_crontab_job_id_t id,
                                        struct mg_str at, bool enable,
                                        struct mg_str action,
                                        struct mg_str payload, void *userdata);

/*
 * Prototype for a job handler to be registered with
 * `mgos_crontab_register_handler()`.
 */
typedef void (*mgos_crontab_cb)(struct mg_str action, struct mg_str payload,
                                void *userdata);

/*
 * Add a new job. Passed string data is not retained. If `pid` is not NULL,
 * resulting job id is written there.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_crontab_job_add(struct mg_str at, bool enable, struct mg_str action,
                          struct mg_str payload, mgos_crontab_job_id_t *pid,
                          char **perr);

/*
 * Edit a job by its id. Passed string data is not retained.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_crontab_job_edit(mgos_crontab_job_id_t id, struct mg_str at,
                           bool enable, struct mg_str action,
                           struct mg_str payload, char **perr);

/*
 * Remove a job by its id.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_crontab_job_remove(mgos_crontab_job_id_t id, char **perr);

/*
 * Get job details by the job id. All output pointers (`at`, `enable`, `action`,
 * `payload`) are optional (allowed to be NULL). For non-NULL string outputs
 * (`at`, `action` and `payload`), the memory is allocated separately and
 * the caller should free it.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_crontab_job_get(mgos_crontab_job_id_t id, struct mg_str *at,
                          bool *enable, struct mg_str *action,
                          struct mg_str *payload, char **perr);

/*
 * Iterate over all jobs in crontab, see `mgos_crontab_iterate_cb` for details.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_crontab_iterate(mgos_crontab_iterate_cb cb, void *userdata,
                          char **perr);

/*
 * Add a handler for the given string action
 *
 * Example:
 *
 * ```c
 * static void my_foo_cb(struct mg_str action,
 *                       struct mg_str payload, void *userdata) {
 *   LOG(LL_INFO, ("Crontab foo job fired! Payload: %.*s", payload.len,
 *payload.p));
 *   (void) action;
 *   (void) userdata;
 * }
 *
 * // Somewhere else:
 * mgos_crontab_register_handler("foo", my_foo_cb, NULL);
 * ```
 *
 * The code above maps action `foo` in the JSON to the callback `my_foo_cb`.
 */
void mgos_crontab_register_handler(struct mg_str action, mgos_crontab_cb cb,
                                   void *userdata);

/*
 * Calculate the next fire date after the specified date, using crontab ID
 * (returned by all cron RPC methods)
 */
time_t mgos_crontab_get_next_invocation(mgos_crontab_job_id_t id, time_t date);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_CRONTAB_SRC_CRONTAB_H_ */
