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

#include <stdbool.h>

#include "frozen.h"

#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "cc32xx_fs.h"
#include "cc32xx_vfs_fs_slfs.h"

#ifdef MGOS_HAVE_OTA_COMMON
#include "cc3200_updater.h"
#endif
#include "cc3200_vfs_dev_slfs_container.h"

bool cc3200_fs_container_mount(const char *path, const char *container_prefix) {
  char dev_opts[100];
  struct json_out out = JSON_OUT_BUF(dev_opts, sizeof(dev_opts));
  json_printf(&out, "{prefix: %Q}", container_prefix);
  if (!mgos_vfs_mount(path, MGOS_DEV_TYPE_SLFS_CONTAINER, dev_opts,
                      MGOS_VFS_FS_TYPE_SPIFFS, "")) {
    return false;
  }
  return true;
}

bool mgos_core_fs_init(void) {
  if (!(
#ifdef MGOS_HAVE_OTA_COMMON
          cc3200_fs_container_mount("/",
                                    cc3200_upd_get_fs_container_prefix()) &&
#else
          cc3200_fs_container_mount("/", "spiffs.img.0") &&
#endif
          cc32xx_fs_slfs_mount("/slfs"))) {
    return false;
  }
  return true;
}

bool mgos_vfs_common_init(void) {
  return (cc3200_vfs_dev_slfs_container_register_type() &&
          cc32xx_vfs_fs_slfs_register_type());
}
