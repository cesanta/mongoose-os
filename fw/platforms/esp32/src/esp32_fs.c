/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp32/src/esp32_fs.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_flash_encrypt.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_vfs.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "frozen/frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "fw/platforms/esp32/src/esp32_vfs_dev_partition.h"

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

bool esp32_fs_init(void) {
  esp_vfs_t esp_vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    /* ESP API uses void * as first argument, hence all the ugly casts. */
    .open = mgos_vfs_open,
    .close = mgos_vfs_close,
    .read = mgos_vfs_read,
    .write = mgos_vfs_write,
    .stat = mgos_vfs_stat,
    .fstat = mgos_vfs_fstat,
    .lseek = mgos_vfs_lseek,
    .rename = mgos_vfs_rename,
    .unlink = mgos_vfs_unlink,
#if MG_ENABLE_DIRECTORY_LISTING
    .opendir = mgos_vfs_opendir,
    .readdir = mgos_vfs_readdir,
    .closedir = mgos_vfs_closedir,
#endif
  };
  if (esp_vfs_register("", &esp_vfs, NULL) != ESP_OK) {
    LOG(LL_ERROR, ("ESP VFS registration failed"));
    return false;
  }
#if CS_SPIFFS_ENABLE_ENCRYPTION
  if (esp_flash_encryption_enabled() && !esp32_fs_crypt_init()) {
    LOG(LL_ERROR, ("Failed to initialize FS encryption key"));
    return false;
  }
#endif
  const esp_partition_t *fs_part =
      esp32_find_fs_for_app_slot(esp32_get_boot_slot());
  if (fs_part == NULL) {
    LOG(LL_ERROR, ("No FS partition"));
    return false;
  }
  return esp32_vfs_dev_partition_register_type() &&
         mgos_vfs_fs_spiffs_register_type() &&
         esp32_fs_mount_part(fs_part->label, "/");
}

bool esp32_fs_mount_part(const char *label, const char *path) {
  bool encrypt = false;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  encrypt = esp_flash_encryption_enabled();
#endif
  char dev_opts[100], fs_opts[100];
  struct json_out out1 = JSON_OUT_BUF(dev_opts, sizeof(dev_opts));
  json_printf(&out1, "{label: %Q, subtype: %d}", label,
              ESP_PARTITION_SUBTYPE_DATA_SPIFFS);
  struct json_out out2 = JSON_OUT_BUF(fs_opts, sizeof(fs_opts));
  json_printf(&out2, "{encr: %B}", encrypt);
  if (!mgos_vfs_mount(path, MGOS_VFS_DEV_TYPE_ESP32_PARTITION, dev_opts,
                      MGOS_VFS_FS_TYPE_SPIFFS, fs_opts)) {
    return false;
  }
  return true;
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
