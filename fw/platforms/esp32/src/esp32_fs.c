/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"

#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"

#include "fw/platforms/esp32/src/esp32_fs.h"

#define MIOT_SPIFFS_MAX_OPEN_FILES 8
#define MIOT_SPIFFS_BLOCK_SIZE SPI_FLASH_SEC_SIZE
#define MIOT_SPIFFS_ERASE_SIZE SPI_FLASH_SEC_SIZE
#define MIOT_SPIFFS_PAGE_SIZE (SPI_FLASH_SEC_SIZE / 16)

struct mount_info {
  spiffs fs;
  u8_t work[2 * MIOT_SPIFFS_PAGE_SIZE];
  u8_t fds[MIOT_SPIFFS_MAX_OPEN_FILES * sizeof(spiffs_fd)];
  const esp_partition_t *part;
};

static s32_t esp32_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  if (size > m->part->size || addr + size > m->part->address + m->part->size) {
    ESP_LOGE("fs", "Invalid read args: %u @ %u", size, addr);
    return SPIFFS_ERR_NOT_READABLE;
  }
  esp_err_t r = spi_flash_read(m->part->address + addr, dst, size);
  if (r == ESP_OK) {
    ESP_LOGD("fs", "Read %u @ %d = %d", size, addr, r);
    return SPIFFS_OK;
  } else {
    ESP_LOGE("fs", "Read %u @ %d = %d", size, addr, r);
    return SPIFFS_ERR_NOT_READABLE;
  }
  return SPIFFS_OK;
}

static s32_t esp32_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  if (size > m->part->size || addr + size > m->part->address + m->part->size) {
    ESP_LOGE("fs", "Invalid read args: %u @ %u", size, addr);
    return SPIFFS_ERR_NOT_WRITABLE;
  }
  esp_err_t r = spi_flash_write(m->part->address + addr, src, size);
  if (r == ESP_OK) {
    ESP_LOGD("fs", "Write %u @ %d = %d", size, addr, r);
    return SPIFFS_OK;
  } else {
    ESP_LOGE("fs", "Write %u @ %d = %d", size, addr, r);
    return SPIFFS_ERR_NOT_WRITABLE;
  }
  return SPIFFS_OK;
}

static s32_t esp32_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  /* Sanity checks */
  if (size > m->part->size || addr + size > m->part->address + m->part->size ||
      addr % SPI_FLASH_SEC_SIZE != 0 || size % SPI_FLASH_SEC_SIZE != 0) {
    ESP_LOGE("fs", "Invalid erase args: %u @ %u", size, addr);
    return SPIFFS_ERR_ERASE_FAIL;
  }
  esp_err_t r = spi_flash_erase_range(m->part->address + addr, size);
  if (r == ESP_OK) {
    ESP_LOGD("fs", "Erase %u @ %d = %d", size, addr, r);
    return SPIFFS_OK;
  } else {
    ESP_LOGE("fs", "Erase %u @ %d = %d", size, addr, r);
    return SPIFFS_ERR_ERASE_FAIL;
  }
}

enum miot_init_result esp32_fs_mount(const esp_partition_t *part) {
  s32_t r;
  spiffs_config cfg;
  struct mount_info *m = (struct mount_info *) calloc(1, sizeof(*m));
  if (m == NULL) return MIOT_INIT_OUT_OF_MEMORY;
  ESP_LOGI("fs", "Mounting SPIFFS %u @ 0x%x", part->size, part->address);
  cfg.hal_read_f = esp32_spiffs_read;
  cfg.hal_write_f = esp32_spiffs_write;
  cfg.hal_erase_f = esp32_spiffs_erase;
  cfg.phys_size = part->size;
  cfg.phys_addr = 0;
  cfg.phys_erase_block = MIOT_SPIFFS_ERASE_SIZE;
  cfg.log_block_size = MIOT_SPIFFS_BLOCK_SIZE;
  cfg.log_page_size = MIOT_SPIFFS_PAGE_SIZE;
  m->part = part;
  m->fs.user_data = m;
  r = SPIFFS_mount(&m->fs, &cfg, m->work, m->fds, sizeof(m->fds), NULL, 0,
                   NULL);
  if (r != SPIFFS_OK) {
    ESP_LOGE("fs", "SPIFFS init failed: %d", (int) SPIFFS_errno(&m->fs));
    free(m);
    return MIOT_INIT_FS_INIT_FAILED;
  }
  return MIOT_INIT_OK;
}

enum miot_init_result esp32_fs_init() {
  const esp_partition_t *fs_part = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  if (fs_part == NULL) {
    ESP_LOGI("fs", "No FS partition.");
    return MIOT_INIT_OK; /* No partition - no problem. */
  }
  return esp32_fs_mount(fs_part);
}
