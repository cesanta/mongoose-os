/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_vfs.h"

#include "common/cs_dbg.h"
#include "common/cs_dirent.h"
#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"
#include "common/spiffs/spiffs_vfs.h"

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

static struct mount_info *s_mount = NULL;

static s32_t esp32_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  if (size > m->part->size || addr + size > m->part->address + m->part->size) {
    LOG(LL_ERROR, ("Invalid read args: %u @ %u", size, addr));
    return SPIFFS_ERR_NOT_READABLE;
  }
  esp_err_t r = spi_flash_read(m->part->address + addr, dst, size);
  if (r == ESP_OK) {
    LOG(LL_VERBOSE_DEBUG, ("Read %u @ %d = %d", size, addr, r));
    return SPIFFS_OK;
  } else {
    LOG(LL_ERROR, ("Read %u @ %d = %d", size, addr, r));
    return SPIFFS_ERR_NOT_READABLE;
  }
  return SPIFFS_OK;
}

static s32_t esp32_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  if (size > m->part->size || addr + size > m->part->address + m->part->size) {
    LOG(LL_ERROR, ("Invalid read args: %u @ %u", size, addr));
    return SPIFFS_ERR_NOT_WRITABLE;
  }
  esp_err_t r = spi_flash_write(m->part->address + addr, src, size);
  if (r == ESP_OK) {
    LOG(LL_VERBOSE_DEBUG, ("Write %u @ %d = %d", size, addr, r));
    return SPIFFS_OK;
  } else {
    LOG(LL_ERROR, ("Write %u @ %d = %d", size, addr, r));
    return SPIFFS_ERR_NOT_WRITABLE;
  }
  return SPIFFS_OK;
}

static s32_t esp32_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  /* Sanity checks */
  if (size > m->part->size || addr + size > m->part->address + m->part->size ||
      addr % SPI_FLASH_SEC_SIZE != 0 || size % SPI_FLASH_SEC_SIZE != 0) {
    LOG(LL_ERROR, ("Invalid erase args: %u @ %u", size, addr));
    return SPIFFS_ERR_ERASE_FAIL;
  }
  esp_err_t r = spi_flash_erase_range(m->part->address + addr, size);
  if (r == ESP_OK) {
    LOG(LL_VERBOSE_DEBUG, ("Erase %u @ %d = %d", size, addr, r));
    return SPIFFS_OK;
  } else {
    LOG(LL_ERROR, ("Erase %u @ %d = %d", size, addr, r));
    return SPIFFS_ERR_ERASE_FAIL;
  }
}

enum miot_init_result esp32_fs_mount(const esp_partition_t *part,
                                     struct mount_info **res) {
  s32_t r;
  spiffs_config cfg;
  *res = NULL;
  struct mount_info *m = (struct mount_info *) calloc(1, sizeof(*m));
  if (m == NULL) return MIOT_INIT_OUT_OF_MEMORY;
  LOG(LL_INFO, ("Mounting SPIFFS %u @ 0x%x", part->size, part->address));
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
    LOG(LL_ERROR, ("SPIFFS init failed: %d", (int) SPIFFS_errno(&m->fs)));
    free(m);
    return MIOT_INIT_FS_INIT_FAILED;
  }
  *res = m;
  return MIOT_INIT_OK;
}

static int spiffs_open_p(void *ctx, const char *path, int flags, int mode) {
  return spiffs_vfs_open(&((struct mount_info *) ctx)->fs, path, flags, mode);
}

static int spiffs_close_p(void *ctx, int fd) {
  return spiffs_vfs_close(&((struct mount_info *) ctx)->fs, fd);
}

static ssize_t spiffs_read_p(void *ctx, int fd, void *dst, size_t size) {
  return spiffs_vfs_read(&((struct mount_info *) ctx)->fs, fd, dst, size);
}

static size_t spiffs_write_p(void *ctx, int fd, const void *data, size_t size) {
  return spiffs_vfs_write(&((struct mount_info *) ctx)->fs, fd, data, size);
}

static int spiffs_stat_p(void *ctx, const char *path, struct stat *st) {
  return spiffs_vfs_stat(&((struct mount_info *) ctx)->fs, path, st);
}

static int spiffs_fstat_p(void *ctx, int fd, struct stat *st) {
  return spiffs_vfs_fstat(&((struct mount_info *) ctx)->fs, fd, st);
}

static off_t spiffs_lseek_p(void *ctx, int fd, off_t offset, int whence) {
  return spiffs_vfs_lseek(&((struct mount_info *) ctx)->fs, fd, offset, whence);
}

static int spiffs_rename_p(void *ctx, const char *src, const char *dst) {
  return spiffs_vfs_rename(&((struct mount_info *) ctx)->fs, src, dst);
}

static int spiffs_unlink_p(void *ctx, const char *path) {
  return spiffs_vfs_unlink(&((struct mount_info *) ctx)->fs, path);
}

/* For cs_dirent.c functions */
spiffs *cs_spiffs_get_fs(void) {
  return (s_mount != NULL ? &s_mount->fs : NULL);
}

enum miot_init_result esp32_fs_init() {
  const esp_partition_t *fs_part = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  if (fs_part == NULL) {
    LOG(LL_ERROR, ("No FS partition."));
    return MIOT_INIT_FS_INIT_FAILED;
  }
  struct mount_info *m;
  enum miot_init_result r = esp32_fs_mount(fs_part, &m);
  if (r == MIOT_INIT_OK) {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_CONTEXT_PTR,
        .open_p = &spiffs_open_p,
        .close_p = &spiffs_close_p,
        .read_p = &spiffs_read_p,
        .write_p = &spiffs_write_p,
        .stat_p = &spiffs_stat_p,
        .fstat_p = &spiffs_fstat_p,
        .lseek_p = &spiffs_lseek_p,
        .rename_p = &spiffs_rename_p,
        .unlink_p = &spiffs_unlink_p,
        .link_p = NULL,
    };
    if (esp_vfs_register(ESP_VFS_DEFAULT, &vfs, m) != ESP_OK) {
      r = MIOT_INIT_FS_INIT_FAILED;
    }
  }
  if (r == MIOT_INIT_OK) {
    s_mount = m;
  }
  return r;
}

void esp32_fs_deinit(void) {
  if (s_mount == NULL) return;
  LOG(LL_INFO, ("Unmounting FS"));
  SPIFFS_unmount(&s_mount->fs);
  /* There is currently no way to deregister VFS, so we don't free s_mount. */
}
