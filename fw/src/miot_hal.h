/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_HAL_H_
#define CS_FW_SRC_MIOT_HAL_H_

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
size_t miot_get_heap_size(void);

/* Get system free memory. */
size_t miot_get_free_heap_size(void);

/* Get minimal watermark of the system free memory. */
size_t miot_get_min_free_heap_size(void);

/* Get filesystem memory usage */
size_t miot_get_fs_memory_usage(void);

/* Feed watchdog */
void miot_wdt_feed(void);

/* Set watchdog timeout*/
void miot_wdt_set_timeout(int secs);

/* Enable watchdog */
void miot_wdt_enable(void);

/* Disable watchdog */
void miot_wdt_disable(void);

/* Restart system */
void miot_system_restart(int exit_code);

/* Delay usecs */
void miot_usleep(int usecs);

/* Get storage free space, bytes */
int64_t miot_get_storage_free_space(void);

/*
 * Invoke a callback in the main MIOT event loop.
 * Returns true if the callback has been scheduled for execution.
 */
typedef void (*miot_cb_t)(void *arg);
bool miot_invoke_cb(miot_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_HAL_H_ */
