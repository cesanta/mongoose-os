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

#include "mongoose.h"

#include "mgos_sys_config.h"
#include "mgos_vfs.h"
#include "mgos_vfs_dev.h"

#include "common/cs_dbg.h"

static const char *ns(const char *s) {
  return (s != NULL ? s : "");
}

static bool process_devtab_from_config(void) {
  char *dt = NULL;
  mg_asprintf(&dt, 0,
              "%s %s %s\n"
              "%s %s %s\n"
              "%s %s %s\n"
              "%s %s %s\n"
              "%s %s %s",
              ns(mgos_sys_config_get_devtab_dev0_name()),
              ns(mgos_sys_config_get_devtab_dev0_type()),
              ns(mgos_sys_config_get_devtab_dev0_opts()),
              ns(mgos_sys_config_get_devtab_dev1_name()),
              ns(mgos_sys_config_get_devtab_dev1_type()),
              ns(mgos_sys_config_get_devtab_dev1_opts()),
              ns(mgos_sys_config_get_devtab_dev2_name()),
              ns(mgos_sys_config_get_devtab_dev2_type()),
              ns(mgos_sys_config_get_devtab_dev2_opts()),
              ns(mgos_sys_config_get_devtab_dev3_name()),
              ns(mgos_sys_config_get_devtab_dev3_type()),
              ns(mgos_sys_config_get_devtab_dev3_opts()),
              ns(mgos_sys_config_get_devtab_dev4_name()),
              ns(mgos_sys_config_get_devtab_dev4_type()),
              ns(mgos_sys_config_get_devtab_dev4_opts()));
  bool res = mgos_process_devtab(dt);
  free(dt);
  return res;
}

static bool process_fstab_config_entry(struct mgos_config_fstab_fs0 *cfg,
                                       bool *config_changed) {
  bool res = false;
  if (cfg->dev == NULL || cfg->type == NULL) {
    res = true;
    goto out;
  }
  if (cfg->create && !cfg->created) {
    res = mgos_vfs_mkfs_dev_name(cfg->dev, cfg->type, ns(cfg->opts));
    if (!res) goto out;
    cfg->created = true;
    *config_changed = true;
  }
  if (cfg->path != NULL) {
    res =
        mgos_vfs_mount_dev_name(cfg->path, cfg->dev, cfg->type, ns(cfg->opts));
    if (!res) goto out;
  }
  res = true;
out:
  return (cfg->optional ? true : res);
}

static bool process_fstab_from_config(void) {
  bool config_changed = false;
  bool res =
      (process_fstab_config_entry(
           (struct mgos_config_fstab_fs0 *) mgos_sys_config_get_fstab_fs0(),
           &config_changed) &&
       process_fstab_config_entry(
           (struct mgos_config_fstab_fs0 *) mgos_sys_config_get_fstab_fs1(),
           &config_changed) &&
       process_fstab_config_entry(
           (struct mgos_config_fstab_fs0 *) mgos_sys_config_get_fstab_fs2(),
           &config_changed));
  if (res && config_changed) {
    save_cfg(&mgos_sys_config, NULL);
  }
  return res;
}

bool mgos_fstab_init_real(void) {
  return (process_devtab_from_config() && process_fstab_from_config());
}

bool mgos_fstab_init(void) {
#ifndef FSTAB_TEST
  return mgos_fstab_init_real();
#else
  return true;
#endif
}
