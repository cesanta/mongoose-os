#ifndef SJ_V7_EXT_INCLUDED
#define SJ_V7_EXT_INCLUDED

#include <v7.h>

/* Initialize objects and functions provided by v7_ext */
void sj_init_v7_ext(struct v7 *v7);

/* Initialize simple http client */
void sj_init_simple_http_client(struct v7 *v7);
void sj_http_error_callback(struct v7 *v7, v7_val_t cb, int err_no);
void sj_http_success_callback(struct v7 *v7, v7_val_t cb, const char *data,
                              size_t data_len);

/*
 * Invokes a callback and prints a stack trace in case of exception.
 *
 * This function is meant to be invoked by HAL implementations of sj_invoke_cb
 */
void _sj_invoke_cb(struct v7 *, v7_val_t func, v7_val_t this_obj,
                   v7_val_t args);

/* Helper, invokes a callback calls in the context of v7 interpreter */
void sj_invoke_cb0(struct v7 *v7, v7_val_t cb);
void sj_invoke_cb1(struct v7 *v7, v7_val_t cb, v7_val_t arg);
void sj_invoke_cb2(struct v7 *v7, v7_val_t cb, v7_val_t arg1, v7_val_t arg2);

/* HAL */

/* Get system free memory. */
size_t sj_get_free_heap_size();

/* Get filesystem memory usage */
size_t sj_get_fs_memory_usage();

/* Feed watchdog */
void sj_wdt_feed();

/* Restart system */
void sj_system_restart();

/* Delay usecs */
void sj_usleep(int usecs);

#endif
