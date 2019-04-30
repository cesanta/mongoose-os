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

#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "miniz.h"

#include "mgos_boot_cfg.h"
#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_internal.h"

#include "stm32_flash.h"
#include "stm32_vfs_dev_flash.h"

#include "stm32_sdk_hal.h"

#define STM32_ROOT_DEV_NAME "fs0"

#if MGOS_ROOT_FS_EXTRACT
extern const unsigned char fs_zip[];
extern unsigned int fs_zip_len;

static bool stm32_fs_extract(void) {
  bool res = false;
  FILE *fp = NULL;
  void *data = NULL;
  mz_zip_archive zip = {0};
  mz_bool zs = mz_zip_reader_init_mem(&zip, &fs_zip[0], fs_zip_len, 0);
  if (!zs) return false;
  int num_files = (int) mz_zip_reader_get_num_files(&zip);
  for (int i = 0; i < num_files; i++) {
    mz_zip_archive_file_stat zfst;
    if (!mz_zip_reader_file_stat(&zip, i, &zfst)) goto out;
    LOG(LL_INFO, ("%s, size: %d, csize: %d", zfst.m_filename,
                  (int) zfst.m_uncomp_size, (int) zfst.m_comp_size));
    // We have plenty of heap at this point, keep it simple.
    size_t uncomp_size = 0;
    data = mz_zip_reader_extract_file_to_heap(&zip, zfst.m_filename,
                                              &uncomp_size, 0);
    if (data == NULL) goto out;
    fp = fopen(zfst.m_filename, "w");
    if (fp == NULL) {
      LOG(LL_ERROR, ("open failed"));
      goto out;
    }
    if (fwrite(data, uncomp_size, 1, fp) != 1) {
      LOG(LL_ERROR, ("write failed"));
      goto out;
    }
    fclose(fp);
    fp = NULL;
    free(data);
    data = NULL;
  }
  res = true;
out:
  mz_zip_reader_end(&zip);
  free(data);
  if (fp != NULL) fclose(fp);
  return res;
}

static bool stm32_fs_create(const char *dev_name, const char *fs_type,
                            const char *fs_opts) {
  LOG(LL_INFO, ("Creating FS..."));
  bool res = false;
  if (!mgos_vfs_mkfs_dev_name(dev_name, fs_type, fs_opts)) goto out;
  if (!mgos_vfs_mount_dev_name("/", dev_name, fs_type, fs_opts)) goto out;
  LOG(LL_INFO, ("Extracting FS..."));
  if (!stm32_fs_extract()) goto out;
  res = mgos_vfs_umount("/");
out:
  return res;
}

/* Note: This a mutable flag on flash. It's a super-cheesy way of doing it,
 * but it'll in a pinch (when we don't have a boot loader). */
const uint8_t f_fs_created = 0xff;

bool stm32_fs_create_if_needed(const char *dev_name, const char *fs_type,
                               const char *fs_opts) {
  bool res = false;
  struct mgos_boot_cfg *bcfg = mgos_boot_cfg_get();
  if (bcfg == NULL) {
    /* Volatile to prevent compiler optimizations. */
    volatile const uint8_t *fp = &f_fs_created;
    int offset = (intptr_t)(((uint8_t *) &f_fs_created) - FLASH_BASE);
    if (*fp != 0) {
      res = stm32_fs_create(dev_name, fs_type, fs_opts);
      if (res) {
        uint8_t val = 0;
        res = stm32_flash_write_region(offset, 1, &val);
      }
    } else {
      res = true;
    }
  } else {
    struct mgos_boot_slot_state *ss = &bcfg->slots[bcfg->active_slot].state;
    if (!(ss->app_flags & MGOS_BOOT_APP_F_FS_CREATED)) {
      res = stm32_fs_create(dev_name, fs_type, fs_opts);
      if (res) {
        ss->app_flags |= MGOS_BOOT_APP_F_FS_CREATED;
        res = mgos_boot_cfg_write(bcfg, true /* dump */);
      }
    } else {
      res = true;
    }
  }
  return res;
}
#else
static bool stm32_fs_create_if_needed(const char *dev_name, const char *fs_type,
                                      const char *fs_opts) {
  (void) dev_name;
  (void) fs_type;
  (void) fs_opts;
  return true;
}
#endif /* MGOS_ROOT_FS_EXTRACT */

bool mgos_core_fs_init(void) {
  const char *dev_name = STM32_ROOT_DEV_NAME;
  const char *fs_type = CS_STRINGIFY_MACRO(MGOS_ROOT_FS_TYPE);
  const char *fs_opts = CS_STRINGIFY_MACRO(MGOS_ROOT_FS_OPTS);
  struct mgos_boot_cfg *bcfg = mgos_boot_cfg_get();
  if (bcfg != NULL) {
    dev_name = bcfg->slots[bcfg->active_slot].cfg.fs_dev;
  }
  return (stm32_fs_create_if_needed(dev_name, fs_type, fs_opts) &&
          mgos_vfs_mount_dev_name("/", dev_name, fs_type, fs_opts));
}

bool mgos_vfs_common_init(void) {
  return stm32_vfs_dev_flash_register_type();
}
