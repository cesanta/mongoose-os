/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "stm32_fs.h"

#include <stm32_sdk_hal.h>

#include "common/cs_dbg.h"
#include "frozen/frozen.h"

#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "stm32_vfs_dev_flash.h"

bool stm32_fs_mount(const char *path, uint32_t addr, uint32_t size) {
  char fs_opts[100];
  struct json_out out = JSON_OUT_BUF(fs_opts, sizeof(fs_opts));
  json_printf(&out, "{addr: %u, size: %u}", addr, size);
  if (!mgos_vfs_mount(path, MGOS_VFS_DEV_TYPE_STM32_FLASH, fs_opts,
                      MGOS_VFS_FS_TYPE_SPIFFS, "")) {
    return false;
  }
  return true;
}

bool stm32_fs_init(void) {
  return stm32_vfs_dev_flash_register_type() &&
         mgos_vfs_fs_spiffs_register_type() &&
         stm32_fs_mount("/", FS_BASE_ADDR, FS_SIZE);
}
