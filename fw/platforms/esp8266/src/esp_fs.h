/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_

#include <stdbool.h>
#include <stdint.h>

#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_ERASE_BLOCK_SIZE 0x10000

bool esp_fs_init(uint32_t root_fs_addr, uint32_t root_fs_size);
bool esp_fs_mount(const char *path, uint32_t addr, uint32_t size);

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_FS_H_ */
