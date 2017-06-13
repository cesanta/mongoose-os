/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_VFS_DEV_SYSFLASH_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_VFS_DEV_SYSFLASH_H_

#include <stdbool.h>

#define MGOS_VFS_DEV_TYPE_SYSFLASH "sysflash"

bool esp_vfs_dev_sysflash_register_type(void);

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_VFS_DEV_SYSFLASH_H_ */
