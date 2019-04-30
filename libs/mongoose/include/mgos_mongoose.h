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
 * This file contains wrappers around low-level Mongoose Library calls.
 *
 * See https://mongoose-os.com/docs/mongoose-os/userguide/intro.md#main-event-loop
 * for the detailed explanation.
 */

#ifndef CS_FW_INCLUDE_MGOS_MONGOOSE_H_
#define CS_FW_INCLUDE_MGOS_MONGOOSE_H_

#include <stdbool.h>

#include "mongoose.h"

#ifndef MGOS_RECV_MBUF_LIMIT
#define MGOS_RECV_MBUF_LIMIT 3072
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Return global event manager */
struct mg_mgr *mgos_get_mgr(void);

/*
 * If there are active connections, calls `mg_mgr_poll` on global event
 * manager. Also calls all registered on-poll callbacks (see
 * `mgos_add_poll_cb()` and friends). Also feeds watchdog if that feature is
 * enabled (see `mgos_wdt_set_feed_on_poll()`). Also reports min free heap size
 * if that feature is enabled (see `mgos_set_enable_min_heap_free_reporting()`)
 */
int mongoose_poll(int ms);

/*
 * On-poll callback; `cb_arg` is an arbitrary pointer given to
 * `mgos_add_poll_cb()`
 */
typedef void (*mgos_poll_cb_t)(void *cb_arg);

/*
 * Add an on-poll callback with an arbitrary argument, see `mongoose_poll()`.
 */
void mgos_add_poll_cb(mgos_poll_cb_t cb, void *cb_arg);

/*
 * Remove an on-poll callback, see `mongoose_poll()`.
 */
void mgos_remove_poll_cb(mgos_poll_cb_t cb, void *cb_arg);

/*
 * Set whether wdt should be fed on each call to `mongoose_poll()`.
 */
void mgos_wdt_set_feed_on_poll(bool enable);

/*
 * Set whether min free heap size should be reported on each call to
 * `mongoose_poll()`.
 */
void mgos_set_enable_min_heap_free_reporting(bool enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_MONGOOSE_H_ */
