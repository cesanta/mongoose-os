/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3220_SRC_CC3220_VFS_DEV_FLASH_H_
#define CS_FW_PLATFORMS_CC3220_SRC_CC3220_VFS_DEV_FLASH_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_DEV_TYPE_FLASH "flash"

bool cc3220_vfs_dev_flash_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3220_SRC_CC3220_VFS_DEV_FLASH_H_ */
