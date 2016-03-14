/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_H_
#define CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "spiffs.h"

struct v7;

int init_fs(struct v7 *v7);
int set_errno(int e);

#ifdef CC3200_FS_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#endif /* CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_H_ */
