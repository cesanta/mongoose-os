/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * See on GitHub:
 * [mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/mgos_hal.h)
 *
 * These interfaces need to be implemented for each hardware platform.
 */

#ifndef CS_FW_SRC_MGOS_HAL_H_
#define CS_FW_SRC_MGOS_HAL_H_

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
void mgos_system_restart(int exit_code);

/* Delay routines */
void mgos_msleep(uint32_t secs);
void mgos_usleep(uint32_t usecs);
extern void (*mgos_nsleep100)(uint32_t n);

void mgos_ints_disable(void);
void mgos_ints_enable(void);

/*
 * Invoke a callback in the main MGOS event loop.
 * Returns true if the callback has been scheduled for execution.
 */
typedef void (*mgos_cb_t)(void *arg);
bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr);

void mgos_lock(void);
void mgos_unlock(void);

struct mgos_rlock_type;
struct mgos_rlock_type *mgos_new_rlock(void);
void mgos_rlock(struct mgos_rlock_type *l);
void mgos_runlock(struct mgos_rlock_type *l);

/* Get the CPU frequency in Hz */
uint32_t mgos_get_cpu_freq(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_HAL_H_ */
