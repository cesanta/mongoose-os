/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp32/src/esp32_updater.h"

#include <stdint.h>
#include <strings.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs.h"

#include "mbedtls/sha1.h"

#include "common/mg_str.h"
#include "common/platform.h"

#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_updater_hal.h"
#include "fw/src/mgos_updater_util.h"
#include "fw/src/mgos_utils.h"

#include "fw/platforms/esp32/src/esp32_fs.h"

/*
 * Since boot loader does not provide a way to store flags, we use NVS.
 * We store a 32-bit uint with old slot, new slot and first boot flag.
 */
#define MGOS_UPDATE_NVS_NAMESPACE "mgos"
#define MGOS_UPDATE_NVS_KEY "update"
#define MGOS_UPDATE_MERGE_FS 0x200
#define MGOS_UPDATE_FIRST_BOOT 0x100
#define MGOS_UPDATE_NVS_VAL(old_slot, new_slot, first_boot, merge_fs)       \
  (((merge_fs) ? MGOS_UPDATE_MERGE_FS : 0) |                                \
   ((first_boot) ? MGOS_UPDATE_FIRST_BOOT : 0) | (((new_slot) &0xf) << 4) | \
   ((old_slot) &0xf))
#define MGOS_UPDATE_OLD_SLOT(v) ((v) &0x0f)
#define MGOS_UPDATE_NEW_SLOT(v) (((v) >> 4) & 0x0f)

#define WRITE_CHUNK_SIZE 32

#define CS_LEN 20 /* SHA1 */
#define CS_HEX_LEN (CS_LEN * 2)
#define CS_HEX_BUF_SIZE (CS_HEX_LEN + 1)

uint32_t g_boot_status = 0;

struct mgos_upd_dev_ctx {
  const char *status_msg;

  const esp_partition_t *cur_app_partition;

  struct json_token app_file_name, app_cs_sha1;
  const esp_partition_t *app_partition;
  esp_ota_handle_t app_ota_handle;

  struct json_token fs_file_name, fs_cs_sha1;
  const esp_partition_t *fs_partition;

  size_t write_offset;
};

struct mgos_upd_dev_ctx *mgos_upd_dev_ctx_create(void) {
  struct mgos_upd_dev_ctx *ctx =
      (struct mgos_upd_dev_ctx *) calloc(1, sizeof(*ctx));
  return ctx;
}

const char *mgos_upd_get_status_msg(struct mgos_upd_dev_ctx *ctx) {
  return ctx->status_msg;
}

static int find_inactive_slot(const esp_partition_t **cur_app_partition,
                              const esp_partition_t **cur_fs_partition,
                              const esp_partition_t **new_app_partition,
                              const esp_partition_t **new_fs_partition,
                              const char **status_msg) {
  *cur_app_partition = esp_ota_get_boot_partition();
  if (*cur_app_partition == NULL) {
    *status_msg = "Not in OTA boot mode";
    return -1;
  }
  int slot = SUBTYPE_TO_SLOT((*cur_app_partition)->subtype);
  *cur_fs_partition = esp32_find_fs_for_app_slot(slot);
  /* Find next OTA slot */
  do {
    slot = (slot + 1) & ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
    esp_partition_subtype_t subtype = ESP_PARTITION_SUBTYPE_OTA(slot);
    if (subtype == (*cur_app_partition)->subtype) break;
    *new_app_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, subtype, NULL);
  } while (*new_app_partition == NULL);
  if (*new_app_partition == NULL) {
    *status_msg = "No app slots";
    return -2;
  }
  *new_fs_partition = esp32_find_fs_for_app_slot(slot);
  if (*new_fs_partition == NULL) {
    *status_msg = "No fs slots";
    return -3;
  }
  return slot;
}

int mgos_upd_begin(struct mgos_upd_dev_ctx *ctx, struct json_token *parts) {
  const esp_partition_t *cur_fs_partition;
  int slot = find_inactive_slot(&ctx->cur_app_partition, &cur_fs_partition,
                                &ctx->app_partition, &ctx->fs_partition,
                                &ctx->status_msg);
  if (slot < 0) {
    return slot;
  }

  if (json_scanf(parts->ptr, parts->len,
                 "{app: {src: %T, cs_sha1: %T}, fs: {src: %T, cs_sha1: %T}}",
                 &ctx->app_file_name, &ctx->app_cs_sha1, &ctx->fs_file_name,
                 &ctx->fs_cs_sha1) != 4) {
    ctx->status_msg = "Incomplete update package";
    return -3;
  }

  LOG(LL_INFO, ("App: %.*s -> %s, FS %.*s -> %s", (int) ctx->app_file_name.len,
                ctx->app_file_name.ptr, ctx->app_partition->label,
                (int) ctx->fs_file_name.len, ctx->fs_file_name.ptr,
                ctx->fs_partition->label));

  return 1;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

static bool compute_checksum(const esp_partition_t *p, size_t len,
                             char *cs_hex) {
  bool ret = false;
  size_t offset = 0;
  mbedtls_sha1_context sha1_ctx;
  unsigned char digest[CS_LEN];
  mbedtls_sha1_init(&sha1_ctx);
  mbedtls_sha1_starts(&sha1_ctx);
  while (offset < len) {
    uint8_t tmp[WRITE_CHUNK_SIZE];
    size_t block_len = len - offset;
    if (block_len > sizeof(tmp)) block_len = sizeof(tmp);
    esp_err_t err = esp_partition_read(p, offset, tmp, block_len);
    if (err != ESP_OK) {
      LOG(LL_ERROR,
          ("Error reading %s at offset %u: %d", p->label, offset, err));
      goto cleanup;
    }
    mbedtls_sha1_update(&sha1_ctx, tmp, block_len);
    offset += block_len;
  }
  mbedtls_sha1_finish(&sha1_ctx, digest);
  bin2hex(digest, CS_LEN, cs_hex);
  cs_hex[CS_HEX_LEN] = '\0';
  ret = true;

cleanup:
  mbedtls_sha1_free(&sha1_ctx);
  return ret;
}

static bool verify_checksum(const esp_partition_t *p, size_t len,
                            const char *exp_sha1, bool critical) {
  char cs_hex[CS_HEX_BUF_SIZE];
  bool ret = compute_checksum(p, len, cs_hex) &&
             (strncmp(cs_hex, exp_sha1, CS_HEX_LEN) == 0);
  LOG((ret || !critical ? LL_DEBUG : LL_ERROR),
      ("%s: %u @ 0x%x, cs_sha1 %s, expected %.*s", p->label, len, p->address,
       cs_hex, CS_HEX_LEN, exp_sha1));
  return ret;
}

enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_dev_ctx *ctx, const struct mgos_upd_file_info *fi) {
  esp_err_t err;
  ctx->write_offset = 0;
  if (strncmp(fi->name, ctx->app_file_name.ptr, ctx->app_file_name.len) == 0) {
    if (verify_checksum(ctx->app_partition, fi->size, ctx->app_cs_sha1.ptr,
                        false /* critical */)) {
      LOG(LL_INFO, ("Skip writing app (digest matches)"));
      return MGOS_UPDATER_SKIP_FILE;
    }
    LOG(LL_INFO, ("Writing app image @ 0x%x", ctx->app_partition->address));
    if (esp_ota_begin(ctx->app_partition, 0, &ctx->app_ota_handle) != ESP_OK) {
      ctx->status_msg = "Failed to start app write";
      return MGOS_UPDATER_ABORT;
    }
    return MGOS_UPDATER_PROCESS_FILE;
  } else if (strncmp(fi->name, ctx->fs_file_name.ptr, ctx->fs_file_name.len) ==
             0) {
    if (verify_checksum(ctx->fs_partition, fi->size, ctx->app_cs_sha1.ptr,
                        false /* critical */)) {
      LOG(LL_INFO, ("Skip writing FS (digest matches)"));
      return MGOS_UPDATER_SKIP_FILE;
    }
    LOG(LL_INFO, ("Writing FS image @ 0x%x", ctx->fs_partition->address));
    err = esp_partition_erase_range(ctx->fs_partition, 0,
                                    ctx->fs_partition->size);
    if (err != ESP_OK) {
      ctx->status_msg = "Failed to start FS write";
      LOG(LL_ERROR, ("%s: %d", ctx->status_msg, err));
      return MGOS_UPDATER_ABORT;
    }
    return MGOS_UPDATER_PROCESS_FILE;
  }
  LOG(LL_DEBUG, ("Not interesting: %s", fi->name));
  return MGOS_UPDATER_SKIP_FILE;
}

int mgos_upd_file_data(struct mgos_upd_dev_ctx *ctx,
                       const struct mgos_upd_file_info *fi,
                       struct mg_str data) {
  esp_err_t err = ESP_FAIL;
  int to_process = (data.len / WRITE_CHUNK_SIZE) * WRITE_CHUNK_SIZE;
  if (to_process > 0) {
    if (strncmp(fi->name, ctx->app_file_name.ptr, ctx->app_file_name.len) ==
        0) {
      err = esp_ota_write(ctx->app_ota_handle, data.p, to_process);
    } else if (strncmp(fi->name, ctx->fs_file_name.ptr,
                       ctx->fs_file_name.len) == 0) {
      err = esp_partition_write(ctx->fs_partition, ctx->write_offset, data.p,
                                to_process);
    }
    if (err != ESP_OK) {
      LOG(LL_ERROR,
          ("Write %d @ %d failed: %d", (int) data.len, ctx->write_offset, err));
      ctx->status_msg = "Failed to write data";
      return -1;
    }
    ctx->write_offset += to_process;
  }
  return to_process;
}

int mgos_upd_file_end(struct mgos_upd_dev_ctx *ctx,
                      const struct mgos_upd_file_info *fi, struct mg_str tail) {
  const esp_partition_t *p = NULL;
  struct json_token *cs_sha1 = NULL;
  assert(tail.len < WRITE_CHUNK_SIZE);
  int ret = -1;
  if (tail.len > 0) {
    char tmp[WRITE_CHUNK_SIZE];
    memset(tmp, 0xff, sizeof(tmp));
    memcpy(tmp, tail.p, tail.len);
    ret = mgos_upd_file_data(ctx, fi, mg_mk_str_n(tmp, sizeof(tmp)));
    if (ret != sizeof(tmp)) {
      ctx->status_msg = "Failed to write tail";
      return -1;
    }
  }
  if (strncmp(fi->name, ctx->app_file_name.ptr, ctx->app_file_name.len) == 0) {
    esp_ota_handle_t oh = ctx->app_ota_handle;
    ctx->app_ota_handle = 0;
    if (esp_ota_end(oh) != ESP_OK) {
      ctx->app_ota_handle = 0;
      ctx->status_msg = "Failed to finalize app write";
      return -1;
    }
    p = ctx->app_partition;
    cs_sha1 = &ctx->app_cs_sha1;
  } else if (strncmp(fi->name, ctx->fs_file_name.ptr, ctx->fs_file_name.len) ==
             0) {
    p = ctx->fs_partition;
    cs_sha1 = &ctx->fs_cs_sha1;
  } else {
    return -123;
  }
  if (!verify_checksum(p, fi->size, cs_sha1->ptr, true /* critical */)) {
    ctx->status_msg = "Digest mismatch";
    return -10;
  }
  return tail.len;
}

static bool set_update_status(int old_slot, int new_slot, bool first_boot,
                              bool merge_fs) {
  bool ret = false;
  nvs_handle h;
  esp_err_t err = nvs_open(MGOS_UPDATE_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK) {
    LOG(LL_ERROR, ("Failed to open NVS: %d", err));
    return false;
  }
  const uint32_t val =
      MGOS_UPDATE_NVS_VAL(old_slot, new_slot, first_boot, merge_fs);
  err = nvs_set_u32(h, MGOS_UPDATE_NVS_KEY, val);
  if (err != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set: %d", err));
    goto cleanup;
  }
  LOG(LL_DEBUG, ("New status: %08x", val));
  g_boot_status = val;
  ret = true;

cleanup:
  nvs_close(h);
  return ret;
}

static uint32_t get_update_status() {
  uint32_t val = 0;
  nvs_handle h;
  esp_err_t err = nvs_open(MGOS_UPDATE_NVS_NAMESPACE, NVS_READONLY, &h);
  if (err != ESP_OK) {
    /* This is normal, meaning no update has taken place yet. */
    if (err != ESP_ERR_NVS_NOT_FOUND) {
      LOG(LL_ERROR, ("Failed to open NVS: %d", err));
    }
    return val;
  }
  err = nvs_get_u32(h, MGOS_UPDATE_NVS_KEY, &val);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    LOG(LL_ERROR, ("Failed to get: %d", err));
    goto cleanup;
  }
  LOG(LL_DEBUG, ("Update status: %08x", val));

cleanup:
  nvs_close(h);
  return val;
}

static bool esp32_set_boot_slot(int slot) {
  const esp_partition_t *p = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_OTA(slot), NULL);
  if (p == NULL) return false;
  LOG(LL_INFO, ("Setting boot partition to %s", p->label));
  return (esp_ota_set_boot_partition(p) == ESP_OK);
}

int mgos_upd_finalize(struct mgos_upd_dev_ctx *ctx) {
  if (!set_update_status(SUBTYPE_TO_SLOT(ctx->cur_app_partition->subtype),
                         SUBTYPE_TO_SLOT(ctx->app_partition->subtype),
                         true /* first_boot */, true /* merge_fs */)) {
    ctx->status_msg = "Failed to set update status";
    return -1;
  }
  if (!esp32_set_boot_slot(SUBTYPE_TO_SLOT(ctx->app_partition->subtype))) {
    ctx->status_msg = "Failed to set boot partition";
    return -1;
  }
  return 1;
}

void mgos_upd_dev_ctx_free(struct mgos_upd_dev_ctx *ctx) {
  if (ctx == NULL) return;
  if (ctx->app_ota_handle != 0) {
    esp_ota_end(ctx->app_ota_handle);
  }
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

static bool copy_partition(const esp_partition_t *src,
                           const esp_partition_t *dst) {
  esp_err_t err;
  if (src->size > dst->size) return false;
  char cs_hex[CS_HEX_BUF_SIZE];
  if (!compute_checksum(src, src->size, cs_hex)) {
    return false;
  }
  if (verify_checksum(dst, src->size, cs_hex, false /* critical */)) {
    LOG(LL_INFO,
        ("%s -> %s: digest matched (%s)", src->label, dst->label, cs_hex));
    return true;
  }
  if ((err = esp_partition_erase_range(dst, 0, src->size)) != ESP_OK) {
    LOG(LL_ERROR, ("%s: erase %u failed: %d", dst->label, src->size, err));
    return false;
  }
  uint32_t offset = 0;
  while (offset < src->size) {
    uint32_t buf[128];
    uint32_t n = sizeof(buf);
    if (n > src->size - offset) n = src->size - offset;
    if ((err = esp_partition_read(src, offset, buf, n)) != ESP_OK) {
      LOG(LL_ERROR, ("%s: read @ %u failed: %d", src->label, offset, err));
      return false;
    }
    if ((err = esp_partition_write(dst, offset, buf, n)) != ESP_OK) {
      LOG(LL_ERROR, ("%s: write @ %u failed: %d", dst->label, offset, err));
      return false;
    }
    offset += n;
  }
  if (!verify_checksum(dst, src->size, cs_hex, true /* critical */)) {
    return false;
  }
  LOG(LL_INFO, ("%s -> %s: copied %u bytes, SHA1 %s", src->label, dst->label,
                offset, cs_hex));
  return true;
}

int mgos_upd_create_snapshot() {
  const esp_partition_t *cur_app_partition, *cur_fs_partition;
  const esp_partition_t *new_app_partition, *new_fs_partition;
  const char *status_msg = NULL;
  int slot =
      find_inactive_slot(&cur_app_partition, &cur_fs_partition,
                         &new_app_partition, &new_fs_partition, &status_msg);
  if (slot < 0) {
    LOG(LL_ERROR, ("%s", status_msg));
    return slot;
  }
  LOG(LL_INFO, ("Snapshot: %s -> %s, %s -> %s", cur_app_partition->label,
                new_app_partition->label, cur_fs_partition->label,
                new_fs_partition->label));
  if (!copy_partition(cur_app_partition, new_app_partition)) return -2;
  if (!copy_partition(cur_fs_partition, new_fs_partition)) return -3;
  LOG(LL_INFO, ("Snapshot created"));
  return slot;
}

bool mgos_upd_boot_get_state(struct mgos_upd_boot_state *bs) {
  memset(bs, 0, sizeof(*bs));
  bs->active_slot = MGOS_UPDATE_NEW_SLOT(g_boot_status);
  bs->revert_slot = MGOS_UPDATE_OLD_SLOT(g_boot_status);
  bs->is_committed = (g_boot_status & MGOS_UPDATE_FIRST_BOOT) == 0;
  return true;
}

bool mgos_upd_boot_set_state(const struct mgos_upd_boot_state *bs) {
  return (set_update_status(bs->revert_slot, bs->active_slot,
                            !bs->is_committed /* first_boot */,
                            false /* merge_fs */) &&
          esp32_set_boot_slot(bs->active_slot));
}

void mgos_upd_boot_revert() {
  int slot = MGOS_UPDATE_OLD_SLOT(g_boot_status);
  if (slot == MGOS_UPDATE_NEW_SLOT(g_boot_status)) return;
  LOG(LL_ERROR, ("Reverting to slot %d", slot));
  set_update_status(slot, slot, false /* first_boot */, false /* merger_fs */);
  esp32_set_boot_slot(slot);
}

void mgos_upd_boot_commit() {
  int slot = MGOS_UPDATE_NEW_SLOT(g_boot_status);
  if (set_update_status(MGOS_UPDATE_OLD_SLOT(g_boot_status), slot,
                        false /* first_boot */, false /* merger_fs */) &&
      esp32_set_boot_slot(slot)) {
    LOG(LL_INFO, ("Committed slot %d", slot));
  } else {
    LOG(LL_ERROR, ("Failed to commit update"));
  }
}

int mgos_upd_apply_update() {
  int ret = -1;
  int old_slot = MGOS_UPDATE_OLD_SLOT(g_boot_status);
  if (MGOS_UPDATE_NEW_SLOT(g_boot_status) == old_slot ||
      !(g_boot_status & MGOS_UPDATE_MERGE_FS)) {
    return 0;
  }
  const esp_partition_t *old_fs_part = esp32_find_fs_for_app_slot(old_slot);
  if (old_fs_part == NULL) {
    LOG(LL_ERROR, ("No old FS partition"));
    return -1;
  }
  struct mount_info *m;
  if (esp32_fs_mount(old_fs_part, &m) != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mount old file system"));
    return -1;
  }
  ret = mgos_upd_merge_spiffs(&m->fs);
  esp32_fs_umount(m);

  return ret;
}

void esp32_updater_early_init() {
  g_boot_status = get_update_status();
  if (!esp32_is_first_boot()) return;
  /*
   * Tombstone the current config. If anything goes wrong between now and
   * commit, next boot will use the old slot.
   */
  uint32_t bs = g_boot_status;
  set_update_status(MGOS_UPDATE_OLD_SLOT(g_boot_status),
                    MGOS_UPDATE_OLD_SLOT(g_boot_status), false /* first_boot */,
                    false /* merge_fs */);
  g_boot_status = bs;
  esp32_set_boot_slot(MGOS_UPDATE_OLD_SLOT(g_boot_status));
}

bool esp32_is_first_boot() {
  return (g_boot_status & MGOS_UPDATE_FIRST_BOOT) != 0;
}
