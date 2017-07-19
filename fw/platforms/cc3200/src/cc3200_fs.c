/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <errno.h>
#include <stdlib.h>

#include "frozen/frozen.h"

#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "fw/platforms/cc3200/src/cc3200_vfs_dev_slfs_container.h"
#include "fw/platforms/cc3200/src/cc3200_vfs_fs_slfs.h"

bool cc3200_fs_mount(const char *path, const char *container_prefix) {
  char dev_opts[100];
  struct json_out out = JSON_OUT_BUF(dev_opts, sizeof(dev_opts));
  json_printf(&out, "{prefix: %Q}", container_prefix);
  if (!mgos_vfs_mount(path, MGOS_DEV_TYPE_SLFS_CONTAINER, dev_opts,
                      MGOS_VFS_FS_TYPE_SPIFFS, "")) {
    return false;
  }
  return true;
}

bool cc3200_fs_init(const char *root_container_prefix) {
  return cc3200_vfs_dev_slfs_container_register_type() &&
         cc3200_vfs_fs_slfs_register_type() &&
         mgos_vfs_fs_spiffs_register_type() &&
         cc3200_fs_mount("/", root_container_prefix) &&
         mgos_vfs_mount("/slfs", NULL, NULL, MGOS_VFS_FS_TYPE_SLFS, "");
}
