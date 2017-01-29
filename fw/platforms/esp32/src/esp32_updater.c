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
#define MGOS_UPDATE_NVS_VAL(old_slot, new_slot, first_boot) \
  (((first_boot) ? 0x100 : 0) | (((new_slot) &0xf) << 4) | ((old_slot) &0xf))
#define MGOS_UPDATE_OLD_SLOT(v) ((v) &0x0f)
#define MGOS_UPDATE_NEW_SLOT(v) (((v) >> 4) & 0x0f)
#define MGOS_UPDATE_FIRST_BOOT 0x100

#define WRITE_CHUNK_SIZE 32

uint32_t g_boot_status = 0;

struct mgos_upd_ctx {
  const char *status_msg;

  const esp_partition_t *cur_app_partition;

  struct json_token app_file_name, app_cs_sha1;
  const esp_partition_t *app_partition;
  esp_ota_handle_t app_ota_handle;

  struct json_token fs_file_name, fs_cs_sha1;
  const esp_partition_t *fs_partition;

  size_t write_offset;
};

struct mgos_upd_ctx *mgos_upd_ctx_create(void) {
  struct mgos_upd_ctx *ctx = (struct mgos_upd_ctx *) calloc(1, sizeof(*ctx));
  return ctx;
}

const char *mgos_upd_get_status_msg(struct mgos_upd_ctx *ctx) {
  return ctx->status_msg;
}

int mgos_upd_begin(struct mgos_upd_ctx *ctx, struct json_token *parts) {
  ctx->cur_app_partition = esp_ota_get_boot_partition();
  if (ctx->cur_app_partition == NULL) {
    ctx->status_msg = "Not in OTA boot mode";
    return -1;
  }
  /* Find next OTA slot */
  int slot = SUBTYPE_TO_SLOT(ctx->cur_app_partition->subtype);
  do {
    slot = (slot + 1) & ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
    esp_partition_subtype_t subtype = ESP_PARTITION_SUBTYPE_OTA(slot);
    if (subtype == ctx->cur_app_partition->subtype) break;
    ctx->app_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, subtype, NULL);
  } while (ctx->app_partition == NULL);
  if (ctx->app_partition == NULL) {
    ctx->status_msg = "No app slots";
    return -1;
  }
  ctx->fs_partition = esp32_find_fs_for_app_slot(slot);
  if (ctx->fs_partition == NULL) {
    ctx->status_msg = "No fs slots";
    return -2;
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

enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_ctx *ctx, const struct mgos_upd_file_info *fi) {
  esp_err_t err;
  ctx->write_offset = 0;
  if (strncmp(fi->name, ctx->app_file_name.ptr, ctx->app_file_name.len) == 0) {
    LOG(LL_INFO, ("Writing app image @ 0x%x", ctx->app_partition->address));
    if (esp_ota_begin(ctx->app_partition, 0, &ctx->app_ota_handle) != ESP_OK) {
      ctx->status_msg = "Failed to start app write";
      return MGOS_UPDATER_ABORT;
    }
    return MGOS_UPDATER_PROCESS_FILE;
  } else if (strncmp(fi->name, ctx->fs_file_name.ptr, ctx->fs_file_name.len) ==
             0) {
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

int mgos_upd_file_data(struct mgos_upd_ctx *ctx,
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

void bin2hex(const uint8_t *src, int src_len, char *dst);

static bool verify_sha1(const esp_partition_t *p, size_t len,
                        struct json_token *exp_sha1) {
  bool ret = false;
  size_t offset = 0;
  mbedtls_sha1_context sha1_ctx;
  unsigned char digest[20];
  char digest_hex[41];
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
  bin2hex(digest, 20, digest_hex);
  digest_hex[40] = '\0';
  ret = (exp_sha1->len == 40 && strncmp(digest_hex, exp_sha1->ptr, 40) == 0);
  LOG((ret ? LL_DEBUG : LL_ERROR),
      ("%s: %u @ 0x%x, cs_sha1 %s, expected %.*s", p->label, len, p->address,
       digest_hex, (int) exp_sha1->len, exp_sha1->ptr));
cleanup:
  mbedtls_sha1_free(&sha1_ctx);
  return ret;
}

int mgos_upd_file_end(struct mgos_upd_ctx *ctx,
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
  if (!verify_sha1(p, fi->size, cs_sha1)) {
    ctx->status_msg = "Digest mismatch";
    return -10;
  }
  return tail.len;
}

static bool set_update_status(int old_slot, int new_slot, bool first_boot) {
  bool ret = false;
  nvs_handle h;
  esp_err_t err = nvs_open(MGOS_UPDATE_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK) {
    LOG(LL_ERROR, ("Failed to open NVS: %d", err));
    return false;
  }
  const uint32_t val = MGOS_UPDATE_NVS_VAL(old_slot, new_slot, first_boot);
  err = nvs_set_u32(h, MGOS_UPDATE_NVS_KEY, val);
  if (err != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set: %d", err));
    goto cleanup;
  }
  LOG(LL_DEBUG, ("New status: %08x", val));
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

int mgos_upd_finalize(struct mgos_upd_ctx *ctx) {
  if (!set_update_status(SUBTYPE_TO_SLOT(ctx->cur_app_partition->subtype),
                         SUBTYPE_TO_SLOT(ctx->app_partition->subtype),
                         true /* first_boot */)) {
    ctx->status_msg = "Failed to set update status";
    return -1;
  }
  if (!esp32_set_boot_slot(SUBTYPE_TO_SLOT(ctx->app_partition->subtype))) {
    ctx->status_msg = "Failed to set boot partition";
    return -1;
  }
  return 1;
}

void mgos_upd_ctx_free(struct mgos_upd_ctx *ctx) {
  if (ctx == NULL) return;
  if (ctx->app_ota_handle != 0) {
    esp_ota_end(ctx->app_ota_handle);
  }
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

int mgos_upd_create_snapshot() {
  /* TODO(rojer): Implement. */
  return -1;
}

void mgos_upd_boot_revert() {
  int slot = MGOS_UPDATE_OLD_SLOT(g_boot_status);
  LOG(LL_ERROR, ("Reverting to slot %d", slot));
  set_update_status(slot, slot, false /* first_boot */);
  esp32_set_boot_slot(slot);
}

void mgos_upd_boot_commit() {
  int slot = MGOS_UPDATE_NEW_SLOT(g_boot_status);
  if (set_update_status(slot, slot, false /* first_boot */) &&
      esp32_set_boot_slot(slot)) {
    LOG(LL_INFO, ("Committed slot %d", slot));
  } else {
    LOG(LL_INFO, ("Failed to commit update"));
  }
}

int mgos_upd_apply_update() {
  int ret = -1;
  int old_slot = MGOS_UPDATE_OLD_SLOT(g_boot_status);
  if (MGOS_UPDATE_NEW_SLOT(g_boot_status) == old_slot) {
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
  set_update_status(MGOS_UPDATE_OLD_SLOT(g_boot_status),
                    MGOS_UPDATE_OLD_SLOT(g_boot_status),
                    false /* first_boot */);
  esp32_set_boot_slot(MGOS_UPDATE_OLD_SLOT(g_boot_status));
}

bool esp32_is_first_boot() {
  return (g_boot_status & MGOS_UPDATE_FIRST_BOOT) != 0;
}
