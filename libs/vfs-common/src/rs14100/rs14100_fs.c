/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "mgos.h"

#ifdef MGOS_HAVE_BOOTLOADER
#include "mgos_boot_cfg.h"
#endif

#include "rs14100_vfs_dev_qspi_flash.h"

bool mgos_core_fs_init(void) {
  const char *dev_name = "fs0";
  const char *fs_type = CS_STRINGIFY_MACRO(MGOS_ROOT_FS_TYPE);
  const char *fs_opts = CS_STRINGIFY_MACRO(MGOS_ROOT_FS_OPTS);
#ifdef MGOS_HAVE_BOOTLOADER
  struct mgos_boot_cfg *bcfg = mgos_boot_cfg_get();
  if (bcfg != NULL) {
    dev_name = bcfg->slots[bcfg->active_slot].cfg.fs_dev;
  }
#endif
  return mgos_vfs_mount_dev_name("/", dev_name, fs_type, fs_opts);
}

bool mgos_vfs_common_init(void) {
  return rs14100_vfs_dev_qspi_flash_register_type();
}
