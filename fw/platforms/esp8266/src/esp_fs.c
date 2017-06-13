/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_fs.h"

#include "frozen/frozen.h"

#include "fw/src/mgos_vfs.h"
#include "fw/src/mgos_vfs_fs_spiffs.h"

#include "fw/platforms/esp8266/src/esp_vfs_dev_sysflash.h"

bool esp_fs_mount(const char *path, uint32_t addr, uint32_t size) {
  char fs_opts[100];
  struct json_out out = JSON_OUT_BUF(fs_opts, sizeof(fs_opts));
  json_printf(&out, "{addr: %u, size: %u}", addr, size);
  if (!mgos_vfs_mount(path, MGOS_VFS_DEV_TYPE_SYSFLASH, "",
                      MGOS_VFS_FS_TYPE_SPIFFS, fs_opts)) {
    return false;
  }
  return true;
}

bool esp_fs_init(uint32_t root_fs_addr, uint32_t root_fs_size) {
  return mgos_vfs_fs_spiffs_register_type() &&
         esp_vfs_dev_sysflash_register_type() &&
         esp_fs_mount("/", root_fs_addr, root_fs_size);
}
