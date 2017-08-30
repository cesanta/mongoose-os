/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_FS_SLFS_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_FS_SLFS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_FS_TYPE_SLFS "SLFS"

bool cc32xx_vfs_fs_slfs_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_FS_SLFS_H_ */
