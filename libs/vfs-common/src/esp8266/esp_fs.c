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

#include "esp_fs.h"

#include "frozen.h"

#include "mgos.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev_part.h"
#include "mgos_vfs_fs_spiffs.h"

#include "esp_rboot.h"
#include "esp_vfs_dev_sysflash.h"

bool esp_fs_mount(uint32_t addr, uint32_t size, const char *dev_name,
                  const char *path) {
  char dev_opts[100];
  struct json_out out = JSON_OUT_BUF(dev_opts, sizeof(dev_opts));
  json_printf(&out, "{dev: %Q, offset: %u, size: %u}",
              ESP8266_SYSFLASH_DEV_NAME, addr, size);
  return (mgos_vfs_dev_create_and_register(MGOS_VFS_DEV_TYPE_PART, dev_opts,
                                           dev_name) &&
          mgos_vfs_mount_dev_name(path, dev_name, MGOS_VFS_FS_TYPE_SPIFFS, ""));
}

bool mgos_core_fs_init(void) {
  rboot_config *bcfg = get_rboot_config();
  uint32_t root_fs_addr = bcfg->fs_addresses[bcfg->current_rom];
  uint32_t root_fs_size = bcfg->fs_sizes[bcfg->current_rom];
  return (
      mgos_vfs_dev_create_and_register(MGOS_VFS_DEV_TYPE_SYSFLASH, "",
                                       ESP8266_SYSFLASH_DEV_NAME) &&
      esp_fs_mount(root_fs_addr, root_fs_size, MGOS_VFS_ROOT_DEV_NAME, "/"));
}

bool mgos_vfs_common_init(void) {
  return esp_vfs_dev_sysflash_register_type();
}
