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

#include "fw/platforms/esp8266/src/esp_fs.h"

#include "frozen.h"

#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

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
