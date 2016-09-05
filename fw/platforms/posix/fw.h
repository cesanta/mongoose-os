/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_POSIX_FW_H_
#define CS_FW_PLATFORMS_POSIX_FW_H_

#include "v7/v7.h"

extern struct v7 *v7;
int gpio_poll(void);
void init_fw(struct v7 *_v7);

#endif /* CS_FW_PLATFORMS_POSIX_FW_H_ */
