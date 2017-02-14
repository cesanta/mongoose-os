/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_HAL_H_
#define CS_FW_SRC_MGOS_HAL_H_

/*
 * Interfaces that need to be implemented for each devices.
 */

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

/* Delay usecs */
void mgos_usleep(int usecs);

/*
 * Invoke a callback in the main MGOS event loop.
 * Returns true if the callback has been scheduled for execution.
 */
typedef void (*mgos_cb_t)(void *arg);
bool mgos_invoke_cb(mgos_cb_t cb, void *arg);

void mgos_lock(void);
void mgos_unlock(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_HAL_H_ */
