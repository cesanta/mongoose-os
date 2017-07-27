/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_VFS_DEV_PARTITION_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_VFS_DEV_PARTITION_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_VFS_DEV_TYPE_ESP32_PARTITION "esp32part"

bool esp32_vfs_dev_partition_register_type(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_VFS_DEV_PARTITION_H_ */
