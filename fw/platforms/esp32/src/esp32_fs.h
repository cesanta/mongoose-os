/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_

#include <stdbool.h>

#include "esp_partition.h"

const esp_partition_t *esp32_find_fs_for_app_slot(int app_slot);

bool esp32_fs_crypt_init(void);

bool esp32_fs_init(void);

bool esp32_fs_mount_part(const char *label, const char *path);

#define SUBTYPE_TO_SLOT(st) ((st) -ESP_PARTITION_SUBTYPE_OTA(0))
#define NUM_OTA_SLOTS \
  (ESP_PARTITION_SUBTYPE_APP_OTA_MAX - ESP_PARTITION_SUBTYPE_APP_OTA_MIN)
int esp32_get_boot_slot();

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_ */
