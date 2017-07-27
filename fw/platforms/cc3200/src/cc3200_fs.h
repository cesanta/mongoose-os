/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool cc3200_fs_mount(const char *path, const char *container_prefix);

bool cc3200_fs_init(const char *root_container_prefix);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_H_ */
