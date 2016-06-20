/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_

#include <stdio.h>

int init_fs(const char *container_prefix);
int set_errno(int e);

#ifdef CC3200_FS_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_ */
