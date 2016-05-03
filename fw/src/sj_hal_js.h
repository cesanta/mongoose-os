/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_HAL_JS_H_
#define CS_FW_SRC_SJ_HAL_JS_H_

#ifndef CS_DISABLE_JS

#include "v7/v7.h"

/* Make HTTP call, 0/1 - error/success */
int sj_http_call(struct v7 *v7, const char *url, const char *body,
                 size_t body_len, const char *method, v7_val_t cb);

/*
 * Invokes a callback and prints a stack trace in case of exception.
 *
 * Port specific implementation have to make sure it's executed in the
 * main v7 thread.
 */
void sj_invoke_cb(struct v7 *, v7_val_t func, v7_val_t this_obj, v7_val_t args);

#endif /* CS_DISABLE_JS */

#endif /* CS_FW_SRC_SJ_HAL_JS_H_ */
