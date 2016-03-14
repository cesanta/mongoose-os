/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_POSIX_SMARTJS_H_
#define CS_SMARTJS_PLATFORMS_POSIX_SMARTJS_H_

#include "v7/v7.h"

extern struct v7 *v7;
int gpio_poll();
void init_smartjs(struct v7 *_v7);

#endif /* CS_SMARTJS_PLATFORMS_POSIX_SMARTJS_H_ */
