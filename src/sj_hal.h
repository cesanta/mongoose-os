#ifndef __SMARTJS_HAL_H_
#define __SMARTJS_HAL_H_

/*
 * Interfaces that need to be implemented for each devices.
 */

#include <stdlib.h>
#include <v7.h>
#include "sj_i2c.h"

/* Get system free mamory. */
size_t sj_get_free_heap_size();

/* Get filesystem memory usage */
size_t sj_get_fs_memory_usage();

/* Feed watchdog */
void sj_wdt_feed();

/* Restart system */
void sj_system_restart();

/* Delay usecs */
void sj_usleep(int usecs);

/* Setup timer with msecs timeout and cb as a callback */
void sj_set_timeout(int msecs, v7_val_t *cb);

/* Make HTTP call, 0/1 - error/success */
int sj_http_call(struct v7 *v7, const char *url, const char *body,
                 size_t body_len, const char *method, v7_val_t cb);

/* See sj_i2c.h and sj_spi.h for i2c & spi HAL related functions */

#endif /* __SMARTJS_HAL_H_ */
