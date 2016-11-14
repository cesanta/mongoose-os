/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_

#include <stdbool.h>
#include <stdio.h>

int cc3200_fs_init(const char *container_prefix);
void cc3200_fs_flush(void);
void cc3200_fs_umount(void);

int set_errno(int e);

#ifdef CC3200_FS_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_ */
