/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp32/src/esp32_fs.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_vfs.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/spiffs/spiffs_vfs.h"

#include "fw/src/mgos_hal.h"
#include "fw/platforms/esp32/src/esp32_updater.h"

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
  mgos_wdt_feed();
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
  mgos_wdt_feed();
  esp_err_t r = spi_flash_erase_range(m->part->address + addr, size);
  if (r == ESP_OK) {
    LOG(LL_VERBOSE_DEBUG, ("Erase %u @ %d = %d", size, addr, r));
    return SPIFFS_OK;
  } else {
    LOG(LL_ERROR, ("Erase %u @ %d = %d", size, addr, r));
    return SPIFFS_ERR_ERASE_FAIL;
  }
}

enum mgos_init_result esp32_fs_mount(const esp_partition_t *part,
                                     struct mount_info **res) {
  s32_t r;
  u32_t total, used;
  spiffs_config cfg;
  *res = NULL;
  struct mount_info *m = (struct mount_info *) calloc(1, sizeof(*m));
  if (m == NULL) return MGOS_INIT_OUT_OF_MEMORY;
  LOG(LL_INFO, ("Mounting SPIFFS from %s (%u @ 0x%x)", part->label, part->size,
                part->address));
  cfg.hal_read_f = esp32_spiffs_read;
  cfg.hal_write_f = esp32_spiffs_write;
  cfg.hal_erase_f = esp32_spiffs_erase;
  cfg.phys_size = part->size;
  cfg.phys_addr = 0;
  cfg.phys_erase_block = MGOS_SPIFFS_ERASE_SIZE;
  cfg.log_block_size = MGOS_SPIFFS_BLOCK_SIZE;
  cfg.log_page_size = MGOS_SPIFFS_PAGE_SIZE;
  m->part = part;
  m->fs.user_data = m;
  r = SPIFFS_mount(&m->fs, &cfg, m->work, m->fds, sizeof(m->fds), NULL, 0,
                   NULL);
  if (r != SPIFFS_OK) {
    LOG(LL_ERROR, ("SPIFFS init failed: %d", (int) SPIFFS_errno(&m->fs)));
    free(m);
    return MGOS_INIT_FS_INIT_FAILED;
  } else {
#if CS_SPIFFS_ENABLE_ENCRYPTION
    if (!spiffs_vfs_enc_fs(&m->fs)) {
      return MGOS_INIT_FS_INIT_FAILED;
    }
#endif
    if (SPIFFS_info(&m->fs, &total, &used) == SPIFFS_OK) {
      LOG(LL_INFO,
          ("Total: %u, used: %u, free: %u", total, used, total - used));
    }
  }
  *res = m;
  return MGOS_INIT_OK;
}

size_t mgos_get_fs_size(void) {
  u32_t total, used;
  if (s_mount == NULL ||
      SPIFFS_info(&s_mount->fs, &total, &used) != SPIFFS_OK) {
    return 0;
  }
  return total;
}

size_t mgos_get_free_fs_size(void) {
  u32_t total, used;
  if (s_mount == NULL ||
      SPIFFS_info(&s_mount->fs, &total, &used) != SPIFFS_OK) {
    return 0;
  }
  return total - used;
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

static DIR *spiffs_opendir_p(void *ctx, const char *name) {
  return spiffs_vfs_opendir(&((struct mount_info *) ctx)->fs, name);
}

static struct dirent *spiffs_readdir_p(void *ctx, DIR *dir) {
  return spiffs_vfs_readdir(&((struct mount_info *) ctx)->fs, dir);
}

static int spiffs_closedir_p(void *ctx, DIR *dir) {
  return spiffs_vfs_closedir(&((struct mount_info *) ctx)->fs, dir);
}

/* For cs_dirent.c functions */
spiffs *cs_spiffs_get_fs(void) {
  return (s_mount != NULL ? &s_mount->fs : NULL);
}

const esp_partition_t *esp32_find_fs_for_app_slot(int slot) {
  char ota_fs_part_name[5] = {'f', 's', '_', 0, 0};
  const char *fs_part_name = NULL;
  /*
   * If OTA layout is used, use the corresponding FS partition, otherwise use
   * the first data:spiffs partition.
   */
  if (slot >= 0) {
    ota_fs_part_name[3] = slot + (slot < 10 ? '0' : 'a');
    fs_part_name = ota_fs_part_name;
  }
  return esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, fs_part_name);
}

int esp32_get_boot_slot() {
  const esp_partition_t *p = esp_ota_get_boot_partition();
  if (p == NULL) return -1;
  return SUBTYPE_TO_SLOT(p->subtype);
}

enum mgos_init_result esp32_fs_init() {
  const esp_partition_t *fs_part =
      esp32_find_fs_for_app_slot(esp32_get_boot_slot());
  if (fs_part == NULL) {
    LOG(LL_ERROR, ("No FS partition"));
    return MGOS_INIT_FS_INIT_FAILED;
  }
#if CS_SPIFFS_ENABLE_ENCRYPTION
  if (esp32_fs_crypt_init() != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to initialize FS encryption key"));
    return MGOS_INIT_FS_INIT_FAILED;
  }
#endif
  struct mount_info *m;
  enum mgos_init_result r = esp32_fs_mount(fs_part, &m);
  if (r == MGOS_INIT_OK) {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_CONTEXT_PTR,
        .open_p = spiffs_open_p,
        .close_p = spiffs_close_p,
        .read_p = spiffs_read_p,
        .write_p = spiffs_write_p,
        .stat_p = spiffs_stat_p,
        .fstat_p = spiffs_fstat_p,
        .lseek_p = spiffs_lseek_p,
        .rename_p = spiffs_rename_p,
        .unlink_p = spiffs_unlink_p,
        .link_p = NULL,
        .opendir_p = spiffs_opendir_p,
        .readdir_p = spiffs_readdir_p,
        .closedir_p = spiffs_closedir_p,
    };
    if (esp_vfs_register(ESP_VFS_DEFAULT, &vfs, m) != ESP_OK) {
      r = MGOS_INIT_FS_INIT_FAILED;
    }
  }
  if (r == MGOS_INIT_OK) {
    s_mount = m;
  }
  return r;
}

void esp32_fs_umount(struct mount_info *m) {
  LOG(LL_INFO, ("Unmounting %s", m->part->label));
  SPIFFS_unmount(&m->fs);
  memset(m, 0, sizeof(*m));
  free(m);
}

void esp32_fs_deinit(void) {
  if (s_mount == NULL) return;
  LOG(LL_INFO, ("Unmounting FS"));
  SPIFFS_unmount(&s_mount->fs);
  /* There is currently no way to deregister VFS, so we don't free s_mount. */
}

/*
 * Test code, used to test encryption. TODO(rojer): Create a HW test with it.
 * Note: Tjis code is compiled to avoid rot but is eliminated by linker.
 */
const char *golden =
    "0123456789Lorem ipsum dolor sit amet, consectetur adipiscing elit01234";

void read_range(const char *name, int from, int len) {
  char data[100] = {0};
  FILE *fp = fopen(name, "r");
  setvbuf(fp, NULL, _IOFBF, 30);
  fseek(fp, from, SEEK_SET);
  int n = fread(data, 1, len, fp);
  fclose(fp);
  LOG(LL_INFO, ("%d @ %d => %d '%s'", len, from, n, data));
}

void f_test_read(void) {
  char data[100] = {0};
  int fd = esp_vfs_open(_GLOBAL_REENT, "test.txt", O_RDONLY, 0);
  if (fd < 0) abort();
  for (int pos = 0; pos < 70; pos++) {
    for (int len = 0; len <= 70; len++) {
      memset(data, 0, sizeof(data));
      esp_vfs_lseek(_GLOBAL_REENT, fd, pos, SEEK_SET);
      int exp_n = 70 - pos;
      if (exp_n > len) exp_n = len;
      int exp_new_pos = pos + exp_n;
      int n = esp_vfs_read(_GLOBAL_REENT, fd, data, len);
      int new_pos = esp_vfs_lseek(_GLOBAL_REENT, fd, 0, SEEK_CUR);
      LOG(LL_INFO,
          ("%d %d %d %d %d %d", pos, len, n, exp_n, new_pos, exp_new_pos));
      if (n != exp_n) abort();
      if (memcmp(golden + pos, data, exp_n) != 0) {
        LOG(LL_ERROR, ("data error, got: '%.*s'", (int) n, data));
        abort();
      }
    }
    mgos_wdt_feed();
  }
  esp_vfs_close(_GLOBAL_REENT, fd);
  LOG(LL_INFO, ("read test ok"));
}

void verify_file_contents(const char *file, const char *exp_contents,
                          int exp_n) {
  size_t n = 0;
  char *data = cs_read_file("test2.txt", &n);
  if (exp_n < 0) exp_n = strlen(exp_contents);
  if (data == NULL || n != exp_n) {
    LOG(LL_ERROR, ("Expected %d bytes, got %d", (int) exp_n, (int) n));
    abort();
  }
  if (memcmp(data, golden, exp_n) != 0) {
    LOG(LL_ERROR, ("data error, got: '%.*s'", (int) n, data));
    abort();
  }
  free(data);
}

void f_test_write(void) {
  {
    int fd = esp_vfs_open(_GLOBAL_REENT, "test2.txt",
                          O_WRONLY | O_TRUNC | O_CREAT, 0);
    if (fd < 0) abort();
    int n = esp_vfs_write(_GLOBAL_REENT, fd, golden, 68);
    if (n != 68) abort();
    esp_vfs_close(_GLOBAL_REENT, fd);
  }
  verify_file_contents("test2.txt", golden, 68);
  {
    int fd = esp_vfs_open(_GLOBAL_REENT, "test2.txt", O_WRONLY | O_APPEND, 0);
    if (fd < 0) abort();
    int n = esp_vfs_write(_GLOBAL_REENT, fd, golden + 68, 2);
    if (n != 2) abort();
    esp_vfs_close(_GLOBAL_REENT, fd);
  }
  verify_file_contents("test2.txt", golden, 70);
  {
    int fd = esp_vfs_open(_GLOBAL_REENT, "test2.txt", O_WRONLY | O_TRUNC, 0);
    if (fd < 0) abort();
    int n = esp_vfs_write(_GLOBAL_REENT, fd, golden, 18);
    if (n != 18) abort();
    esp_vfs_close(_GLOBAL_REENT, fd);
  }
  verify_file_contents("test2.txt", golden, 18);
  LOG(LL_INFO, ("write test ok"));
}
