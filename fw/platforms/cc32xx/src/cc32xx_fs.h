/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_FS_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_FS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool cc32xx_fs_slfs_mount(const char *path);
bool cc32xx_fs_spiffs_container_mount(const char *path, const char *container_prefix);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_FS_H_ */
