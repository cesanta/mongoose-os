/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stm32_fs.h"

#include <stm32_sdk_hal.h>

#include "common/cs_dbg.h"
#include "frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "stm32_vfs_dev_flash.h"

extern const unsigned char fs_bin[];

bool stm32_fs_mount(const char *path, uintptr_t addr, uint32_t size) {
  char fs_opts[100];
  struct json_out out = JSON_OUT_BUF(fs_opts, sizeof(fs_opts));
  json_printf(&out, "{addr: %u, size: %u}", addr, size);
  if (!mgos_vfs_mount(path, MGOS_VFS_DEV_TYPE_STM32_FLASH, fs_opts,
                      MGOS_VFS_FS_TYPE_SPIFFS, "")) {
    return false;
  }
  return true;
}

enum mgos_init_result mgos_fs_init(void) {
  if (!(stm32_vfs_dev_flash_register_type() &&
        mgos_vfs_fs_spiffs_register_type() &&
        stm32_fs_mount("/", ((uintptr_t) &fs_bin[0]) - FLASH_BASE_ADDR,
                       FS_SIZE))) {
    return MGOS_INIT_FS_INIT_FAILED;
  }
  return MGOS_INIT_OK;
}
