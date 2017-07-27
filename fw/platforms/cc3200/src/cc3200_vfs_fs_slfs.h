/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_FS_SLFS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_FS_SLFS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_FS_TYPE_SLFS "SLFS"

bool cc3200_vfs_fs_slfs_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_FS_SLFS_H_ */
