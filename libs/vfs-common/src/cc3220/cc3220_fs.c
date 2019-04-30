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

#include "common/str_util.h"

#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "cc32xx_fs.h"
#include "cc32xx_vfs_fs_slfs.h"

#include "cc3220_vfs_dev_flash.h"

bool mgos_core_fs_init(void) {
  return (mgos_vfs_mount(
             "/", MGOS_DEV_TYPE_FLASH,
             "{offset: " CS_STRINGIFY_MACRO(MGOS_FS_OFFSET) ", "
             "size: " CS_STRINGIFY_MACRO(MGOS_FS_SIZE) ", "
             "image: \"" MGOS_FS_IMG "\"}",
             MGOS_VFS_FS_TYPE_SPIFFS,
             "{bs: " CS_STRINGIFY_MACRO(MGOS_FS_BLOCK_SIZE) ", "
             "ps: " CS_STRINGIFY_MACRO(MGOS_FS_PAGE_SIZE) ", "
             "es: " CS_STRINGIFY_MACRO(MGOS_FS_ERASE_SIZE) "}") &&
         cc32xx_fs_slfs_mount("/slfs"));
}

bool mgos_vfs_common_init(void) {
  return (cc3220_vfs_dev_flash_register_type() &&
          cc32xx_vfs_fs_slfs_register_type());
}
