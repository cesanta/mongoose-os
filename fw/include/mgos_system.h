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
 * See on GitHub:
 * [mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/mgos_hal.h)
 *
 * These interfaces need to be implemented for each hardware platform.
 */

#ifndef CS_FW_INCLUDE_MGOS_SYSTEM_H_
#define CS_FW_INCLUDE_MGOS_SYSTEM_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Get system memory size. */
size_t mgos_get_heap_size(void);

/* Get system free memory. */
size_t mgos_get_free_heap_size(void);

/* Get minimal watermark of the system free memory. */
size_t mgos_get_min_free_heap_size(void);

/* Get filesystem memory usage */
size_t mgos_get_fs_memory_usage(void);

/* Get filesystem size. */
size_t mgos_get_fs_size(void);

/* Get filesystem free space. */
size_t mgos_get_free_fs_size(void);

/* Garbage-collect filesystem */
void mgos_fs_gc(void);

/* Feed watchdog */
void mgos_wdt_feed(void);

/* Set watchdog timeout*/
void mgos_wdt_set_timeout(int secs);

/* Enable watchdog */
void mgos_wdt_enable(void);

/* Disable watchdog */
void mgos_wdt_disable(void);

/* Restart system */
void mgos_system_restart(void);

/* Delay given number of milliseconds */
void mgos_msleep(uint32_t msecs);

/* Delay given number of microseconds */
void mgos_usleep(uint32_t usecs);

extern void (*mgos_nsleep100)(uint32_t n);

/* Disable interrupts */
void mgos_ints_disable(void);

/* Enable interrupts */
void mgos_ints_enable(void);

/* Callback for `mgos_invoke_cb()` */
typedef void (*mgos_cb_t)(void *arg);

/*
 * Invoke a callback in the main MGOS event loop.
 * Returns true if the callback has been scheduled for execution.
 */
bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr);

/* Get the CPU frequency in Hz */
uint32_t mgos_get_cpu_freq(void);

/* Opaque recursive lock type. */
struct mgos_rlock_type;

/* Create a recursive lock. */
struct mgos_rlock_type *mgos_rlock_create(void);

/* Acquire a recursive lock. */
void mgos_rlock(struct mgos_rlock_type *l);

/* Release a recursive lock. */
void mgos_runlock(struct mgos_rlock_type *l);

/* Destroy a recursive lock. */
void mgos_rlock_destroy(struct mgos_rlock_type *l);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_SYSTEM_H_ */
