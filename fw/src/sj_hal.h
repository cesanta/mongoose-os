/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_HAL_H_
#define CS_FW_SRC_SJ_HAL_H_

/*
 * Interfaces that need to be implemented for each devices.
 */

#include <inttypes.h>
#include <stdlib.h>

/* Get system memory size. */
size_t sj_get_heap_size(void);

/* Get system free memory. */
size_t sj_get_free_heap_size(void);

/* Get minimal watermark of the system free memory. */
size_t sj_get_min_free_heap_size(void);

/* Get filesystem memory usage */
size_t sj_get_fs_memory_usage(void);

/* Feed watchdog */
void sj_wdt_feed(void);

/* Set watchdog timeout*/
void sj_wdt_set_timeout(int secs);

/* Enable watchdog */
void sj_wdt_enable(void);

/* Disable watchdog */
void sj_wdt_disable(void);

/* Restart system */
void sj_system_restart(int exit_code);

/* Delay usecs */
void sj_usleep(int usecs);

/* Get storage free space, bytes */
int64_t sj_get_storage_free_space(void);

#endif /* CS_FW_SRC_SJ_HAL_H_ */
