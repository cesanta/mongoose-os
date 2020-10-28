/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef MGOS_HAVE_OTA_COMMON

// For now these are all stubs, just to make things compile.

#include "mgos_ota.h"
#include "mgos_ota_backend.h"
#include "mgos_ota_core.h"

static struct mgos_ota_boot_state s_boot_state = {
    .active_slot = 0,
    .revert_slot = 1,
    .is_committed = true,
};

bool mgos_ota_boot_get_state(struct mgos_ota_boot_state *bs) {
  *bs = s_boot_state;
  return true;
}

bool mgos_ota_boot_set_state(const struct mgos_ota_boot_state *bs) {
  s_boot_state = *bs;
  return true;
}

int mgos_ota_apply_update(void) {
  return 0;
}

void mgos_ota_boot_commit(void) {
  s_boot_state.is_committed = true;
}

void mgos_ota_boot_revert(void) {
  int t = s_boot_state.active_slot;
  s_boot_state.active_slot = s_boot_state.revert_slot;
  s_boot_state.revert_slot = t;
  s_boot_state.is_committed = true;
}

bool mgos_ota_is_first_boot(void) {
  return !s_boot_state.is_committed;
}

int mgos_ota_create_snapshot(void) {
  return -1;
}

static const char *ubuntu_upd_get_status_msg(struct mgos_ota_be_ctx *ctx) {
  (void) ctx;
  return "";
}

static struct mgos_ota_be_ctx *ubuntu_upd_create_ctx(void) {
  return NULL;
}

static enum mgos_ota_result ubuntu_upd_begin(
    struct mgos_ota_be_ctx *ctx, const struct mgos_ota_manifest_info *mi) {
  if (mg_vcmp(&mi->platform, "ubuntu") != 0) {
    return MGOS_UPD_SKIP;
  }
  (void) ctx;
  (void) mi;
  return MGOS_UPD_OK;
}

enum mgos_ota_result ubuntu_upd_file_begin(
    struct mgos_ota_be_ctx *ctx, const struct mgos_ota_file_info *fi) {
  (void) ctx;
  (void) fi;
  return MGOS_UPD_SKIP;
}

int ubuntu_upd_file_data(struct mgos_ota_be_ctx *ctx,
                         const struct mgos_ota_file_info *fi,
                         struct mg_str data) {
  (void) ctx;
  (void) fi;
  return (int) data.len;
}

int ubuntu_upd_file_end(struct mgos_ota_be_ctx *ctx,
                        const struct mgos_ota_file_info *fi,
                        struct mg_str tail) {
  (void) ctx;
  (void) fi;
  return (int) tail.len;
}

enum mgos_ota_result ubuntu_upd_finalize(struct mgos_ota_be_ctx *ctx,
                                         bool *need_reboot) {
  (void) ctx;
  (void) need_reboot;
  return MGOS_UPD_OK;
}

void ubuntu_upd_free_ctx(struct mgos_ota_be_ctx *ctx) {
  (void) ctx;
}

static const struct mgos_ota_backend_if s_ubuntu_upd_if = {
    .create_ctx = ubuntu_upd_create_ctx,
    .get_status_msg = ubuntu_upd_get_status_msg,
    .begin = ubuntu_upd_begin,
    .file_begin = ubuntu_upd_file_begin,
    .file_data = ubuntu_upd_file_data,
    .file_end = ubuntu_upd_file_end,
    .finalize = ubuntu_upd_finalize,
    .free_ctx = ubuntu_upd_free_ctx,
};

void mgos_ota_backend_init(void) {
  mgos_ota_register_backend(&s_ubuntu_upd_if);
}

#endif  // MGOS_HAVE_OTA_COMMON
