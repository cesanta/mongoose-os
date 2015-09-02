#ifndef __SMARTJS_HAL_H_
#define __SMARTJS_HAL_H_

/*
 * Interfaces that need to be implemented for each devices.
 */

#include <stdlib.h>
#include <v7.h>
#include "sj_i2c.h"

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

#endif /* __SMARTJS_HAL_H_ */
