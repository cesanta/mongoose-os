/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_

#include "esp_partition.h"

#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"

#include "fw/src/mgos_init.h"

#define MGOS_SPIFFS_MAX_OPEN_FILES 8
#define MGOS_SPIFFS_BLOCK_SIZE SPI_FLASH_SEC_SIZE
#define MGOS_SPIFFS_ERASE_SIZE SPI_FLASH_SEC_SIZE
#define MGOS_SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 16)

struct mount_info {
  spiffs fs;
  u8_t work[2 * MGOS_SPIFFS_PAGE_SIZE];
  u8_t fds[MGOS_SPIFFS_MAX_OPEN_FILES * sizeof(spiffs_fd)];
  const esp_partition_t *part;
};

const esp_partition_t *esp32_find_fs_for_app_slot(int app_slot);

enum mgos_init_result esp32_fs_mount(const esp_partition_t *part,
                                     struct mount_info **res);
void esp32_fs_umount(struct mount_info *m);

enum mgos_init_result esp32_fs_crypt_init(void);

enum mgos_init_result esp32_fs_init(void);

#define SUBTYPE_TO_SLOT(st) ((st) -ESP_PARTITION_SUBTYPE_OTA(0))
int esp32_get_boot_slot();

void esp32_fs_deinit(void);

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_FS_H_ */
