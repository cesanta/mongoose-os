/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_STM32_VFS_DEV_FLASH_H_
#define CS_FW_PLATFORMS_STM32_STM32_VFS_DEV_FLASH_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_DEV_TYPE_STM32_FLASH "stm32flash"

bool stm32_vfs_dev_flash_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_STM32_VFS_DEV_FLASH_H_ */
