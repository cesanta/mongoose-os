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

/* See sj_i2c.h, sj_spi.h and sj_gpio.h for i2c, spi & gpio HAL related
 * functions */
void sj_exec_with(struct v7 *v7, const char *code, v7_val_t this_obj);

#endif /* __SMARTJS_HAL_H_ */
