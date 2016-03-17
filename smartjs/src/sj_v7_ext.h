/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_V7_EXT_H_
#define CS_SMARTJS_SRC_SJ_V7_EXT_H_

#include "v7/v7.h"

/* Initialize objects and functions provided by v7_ext */
void sj_v7_ext_api_setup(struct v7 *v7);

/* Initialize `Sys.*` */
void sj_init_sys(struct v7 *v7);

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

void sj_invoke_cb0_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj);
void sj_invoke_cb1_this(struct v7 *v7, v7_val_t, v7_val_t, v7_val_t);
void sj_invoke_cb2_this(struct v7 *v7, v7_val_t, v7_val_t, v7_val_t, v7_val_t);

/* Prints an exception to stdout or stderr depending on debug mode */
void sj_print_exception(struct v7 *v7, v7_val_t exc, const char *msg);

/* HAL */

/* Get system free memory. */
size_t sj_get_free_heap_size();

/* Get minimal watermark of the system free memory. */
size_t sj_get_min_free_heap_size();

/* Get filesystem memory usage */
size_t sj_get_fs_memory_usage();

/* Feed watchdog */
void sj_wdt_feed();

/* Set watchdog timeout*/
void sj_wdt_set_timeout(int secs);

/* Enable watchdog */
void sj_wdt_enable();

/* Disable watchdog */
void sj_wdt_disable();

/* Restart system */
void sj_system_restart(int exit_code);

/* Delay usecs */
void sj_usleep(int usecs);

#endif /* CS_SMARTJS_SRC_SJ_V7_EXT_H_ */
