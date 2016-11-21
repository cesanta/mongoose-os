/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements mg_upd interface.defined in miot_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>

#include "common/platform.h"
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/cc3200_crypto.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container_meta.h"
#include "fw/platforms/cc3200/src/cc3200_main_task.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_updater_hal.h"
#include "fw/src/miot_updater_util.h"
#include "fw/src/miot_utils.h"

#if MIOT_ENABLE_UPDATER

struct miot_upd_ctx {
  struct json_token *parts;
  int cur_boot_cfg_idx;
  int new_boot_cfg_idx;
  char app_image_file[MAX_APP_IMAGE_FILE_LEN];
  uint32_t app_load_addr;
  char fs_container_file[MAX_FS_CONTAINER_FNAME_LEN + 3];
  uint32_t fs_size, fs_block_size, fs_page_size, fs_erase_size;
  struct json_token cur_part;
  const _u8 *cur_fn;
  _i32 cur_fh;
  const char *status_msg;
};

struct miot_upd_ctx *miot_upd_ctx_create(void) {
  struct miot_upd_ctx *ctx = (struct miot_upd_ctx *) calloc(1, sizeof(*ctx));
  ctx->cur_fh = -1;
  return ctx;
}

const char *miot_upd_get_status_msg(struct miot_upd_ctx *ctx) {
  return ctx->status_msg;
}

int miot_upd_begin(struct miot_upd_ctx *ctx, struct json_token *parts) {
  ctx->parts = parts;
  /* We want to make sure device uses boto loader. */
  ctx->cur_boot_cfg_idx = get_active_boot_cfg_idx();
  if (ctx->cur_boot_cfg_idx < 0) {
    ctx->status_msg = "Could not read current boot cfg";
    return -1;
  }
  ctx->new_boot_cfg_idx = get_inactive_boot_cfg_idx();
  return 1;
}

typedef int (*read_file_cb_t)(_u8 *data, int len, void *arg);
static int read_file(const char *fn, int offset, int len, read_file_cb_t cb,
                     void *arg) {
  _i32 fh;
  int r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  if (r < 0) return r;
  while (len > 0) {
    _u8 buf[512];
    int to_read = MIN(len, sizeof(buf));
    r = sl_FsRead(fh, offset, buf, to_read);
    if (r != to_read) break;
    if (cb(buf, to_read, arg) != to_read) break;
    offset += to_read;
    len -= to_read;
  }
  sl_FsClose(fh, NULL, NULL, 0);
  return (len == 0 ? 0 : -1);
}

static int sha1_update_cb(_u8 *data, int len, void *arg) {
  cc3200_hash_update((struct cc3200_hash_ctx *) arg, data, len);
  return len;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

int verify_checksum(const char *fn, int fs, const struct json_token *expected) {
  int r;

  if (expected->len != 40) return -1;

  struct cc3200_hash_ctx ctx;
  cc3200_hash_init(&ctx, CC3200_HASH_ALGO_SHA1);
  if ((r = read_file(fn, 0, fs, sha1_update_cb, &ctx)) < 0) return r;
  _u8 digest[20];
  cc3200_hash_final(&ctx, digest);

  char digest_str[41];
  bin2hex(digest, 20, digest_str);

  DBG(("%s: have %.*s, want %.*s", fn, 40, digest_str, 40, expected->ptr));

  return (strncasecmp(expected->ptr, digest_str, 40) == 0 ? 1 : 0);
}

/* Create file name by appending ".$idx" to prefix. */
static void create_fname(struct mg_str pfx, int idx, char *fn, int len) {
  int l = MIN(len - 3, pfx.len);
  memcpy(fn, pfx.p, l);
  fn[l++] = '.';
  fn[l++] = ('0' + idx);
  fn[l] = '\0';
}

static int prepare_to_write(struct miot_upd_ctx *ctx,
                            const struct miot_upd_file_info *fi,
                            const char *fname, uint32_t falloc,
                            struct json_token *part) {
  struct json_token expected_sha1 = JSON_INVALID_TOKEN;
  json_scanf(part->ptr, part->len, "{cs_sha1: %T}", &expected_sha1);
  if (verify_checksum(fname, fi->size, &expected_sha1) > 0) {
    LOG(LL_INFO, ("Digest matched for %s %u (%.*s)", fname, fi->size,
                  (int) expected_sha1.len, expected_sha1.ptr));
    return 0;
  }
  LOG(LL_INFO, ("Storing %s %u -> %s %u (%.*s)", fi->name, fi->size, fname,
                falloc, (int) expected_sha1.len, expected_sha1.ptr));
  ctx->cur_fn = (const _u8 *) fname;
  sl_FsDel(ctx->cur_fn, 0);
  _i32 r = sl_FsOpen(ctx->cur_fn, FS_MODE_OPEN_CREATE(falloc, 0), NULL,
                     &ctx->cur_fh);
  if (r < 0) {
    ctx->status_msg = "Failed to create file";
    return r;
  }
  return 1;
}

struct find_part_info {
  const char *src;
  struct mg_str *key;
  struct json_token *value;
  char buf[50];
};

static void find_part(void *data, const char *name, size_t name_len,
                      const char *path, const struct json_token *tok) {
  struct find_part_info *info = (struct find_part_info *) data;
  size_t path_len = strlen(path), src_len = strlen(info->src);

  (void) name;
  (void) name_len;

  if (tok->ptr == NULL) {
    /*
     * We're not interested here in the events for which we have no value;
     * namely, JSON_TYPE_OBJECT_START and JSON_TYPE_ARRAY_START
     */
    return;
  }

  /* For matched 'src' attribute, remember parent object path. */
  if (src_len == tok->len && strncmp(info->src, tok->ptr, tok->len) == 0) {
    const char *p = path + path_len;
    while (--p > path + 1) {
      if (*p == '.') break;
    }

    info->key->len = snprintf(info->buf, sizeof(info->buf), "%.*s",
                              (p - path) - 1, path + 1);
    info->key->p = info->buf;
  }

  /*
   * And store parent's object token. These conditionals are triggered
   * in separate callback invocations.
   */
  if (info->value->len == 0 && info->key->len > 0 &&
      info->key->len + 1 == path_len &&
      strncmp(path + 1, info->key->p, info->key->len) == 0) {
    *info->value = *tok;
  }
}

static int tcmp(const struct json_token *tok, const char *str) {
  struct mg_str s = {.p = tok->ptr, .len = tok->len};
  return mg_vcmp(&s, str);
}

enum miot_upd_file_action miot_upd_file_begin(
    struct miot_upd_ctx *ctx, const struct miot_upd_file_info *fi) {
  struct mg_str part_name = MG_MK_STR("");
  enum miot_upd_file_action ret = MIOT_UPDATER_SKIP_FILE;
  struct find_part_info find_part_info = {fi->name, &part_name, &ctx->cur_part};
  ctx->cur_part.len = part_name.len = 0;
  json_walk(ctx->parts->ptr, ctx->parts->len, find_part, &find_part_info);
  if (ctx->cur_part.len == 0) return ret;
  /* Drop any indexes from part name, we'll add our own. */
  while (1) {
    char c = part_name.p[part_name.len - 1];
    if (c != '.' && !(c >= '0' && c <= '9')) break;
    part_name.len--;
  }
  struct json_token type = JSON_INVALID_TOKEN;
  const char *fname = NULL;
  uint32_t falloc = 0;
  json_scanf(ctx->cur_part.ptr, ctx->cur_part.len,
             "{load_addr:%u, falloc:%u, type: %T}", &ctx->app_load_addr,
             &falloc, &type);

  if (falloc == 0) falloc = fi->size;
  if (tcmp(&type, "app") == 0) {
#if CC3200_SAFE_CODE_UPDATE
    /*
     * When safe code update is enabled, we write code to a new file.
     * Otherwise we write to the same slot we're using currently, which is
     * unsafe, makes reverting coide update not possible, but saves space.
     */
    create_fname(part_name, ctx->new_boot_cfg_idx, ctx->app_image_file,
                 sizeof(ctx->app_image_file));
#else
    {
      struct boot_cfg cur_cfg;
      int r = read_boot_cfg(ctx->cur_boot_cfg_idx, &cur_cfg);
      if (r < 0) {
        ctx->status_msg = "Could not read current boot cfg";
        return MIOT_UPDATER_ABORT;
      }
      strncpy(ctx->app_image_file, cur_cfg.app_image_file,
              sizeof(ctx->app_image_file));
    }
#endif
    if (ctx->app_load_addr >= 0x20000000) {
      fname = ctx->app_image_file;
    } else {
      ctx->status_msg = "Bad/missing app load_addr";
      ret = MIOT_UPDATER_ABORT;
    }
  } else if (tcmp(&type, "fs") == 0) {
    json_scanf(
        ctx->cur_part.ptr, ctx->cur_part.len,
        "{fs_size: %u, fs_block_size: %u, fs_page_size: %u, fs_erase_size: %u}",
        &ctx->fs_size, &ctx->fs_block_size, &ctx->fs_page_size,
        &ctx->fs_erase_size);
    if (ctx->fs_size > 0 && ctx->fs_block_size > 0 && ctx->fs_page_size > 0 &&
        ctx->fs_erase_size > 0) {
      char fs_container_prefix[MAX_FS_CONTAINER_PREFIX_LEN];
      create_fname(part_name, ctx->new_boot_cfg_idx, fs_container_prefix,
                   sizeof(fs_container_prefix));
      /* Delete container 1 (if any) so that 0 is the only one. */
      fs_delete_container(fs_container_prefix, 1);
      fs_container_fname(fs_container_prefix, 0,
                         (_u8 *) ctx->fs_container_file);
      fname = ctx->fs_container_file;
      if (fi->size > ctx->fs_size) {
        /* Assume meta has already been added. */
        falloc = fi->size;
      } else {
        falloc = FS_CONTAINER_SIZE(fi->size);
      }
    } else {
      ctx->status_msg = "Missing FS parameters";
      ret = MIOT_UPDATER_ABORT;
    }
  }
  if (fname != NULL) {
    int r = prepare_to_write(ctx, fi, fname, falloc, &ctx->cur_part);
    if (r < 0) {
      LOG(LL_ERROR, ("err = %d", r));
      ret = MIOT_UPDATER_ABORT;
    } else {
      ret = (r > 0 ? MIOT_UPDATER_PROCESS_FILE : MIOT_UPDATER_SKIP_FILE);
    }
  }
  if (ret == MIOT_UPDATER_SKIP_FILE) {
    DBG(("Skipping %s %.*s", fi->name, (int) part_name.len, part_name.p));
  }
  return ret;
}

int miot_upd_file_data(struct miot_upd_ctx *ctx,
                       const struct miot_upd_file_info *fi,
                       struct mg_str data) {
  _i32 r = sl_FsWrite(ctx->cur_fh, fi->processed, (_u8 *) data.p, data.len);
  if (r != data.len) {
    ctx->status_msg = "Write failed";
    r = -1;
  }
  return r;
}

int miot_upd_file_end(struct miot_upd_ctx *ctx,
                      const struct miot_upd_file_info *fi) {
  int r = 1;
  if (ctx->cur_fn == (_u8 *) ctx->fs_container_file) {
    int ret = fs_write_meta(ctx->cur_fh, FS_INITIAL_SEQ, ctx->fs_size,
                            ctx->fs_block_size, ctx->fs_page_size,
                            ctx->fs_erase_size);
    if (ret < 0) {
      ctx->status_msg = "Failed to write fs meta";
      r = ret;
    }
  }
  if (sl_FsClose(ctx->cur_fh, NULL, NULL, 0) != 0) {
    ctx->status_msg = "Close failed";
    r = -1;
  } else {
    struct json_token sha1 = JSON_INVALID_TOKEN;
    json_scanf(ctx->cur_part.ptr, ctx->cur_part.len, "{cs_sha1: %T}", &sha1);
    r = verify_checksum((const char *) ctx->cur_fn, fi->size, &sha1);
    if (r <= 0) {
      ctx->status_msg = "Checksum mismatch";
      r = -1;
    }
  }
  ctx->cur_fh = -1;
  ctx->cur_fn = NULL;
  return r;
}

int miot_upd_finalize(struct miot_upd_ctx *ctx) {
  struct boot_cfg cur_cfg, new_cfg;
  int r = read_boot_cfg(ctx->cur_boot_cfg_idx, &cur_cfg);
  if (r < 0) {
    ctx->status_msg = "Could not read current boot cfg";
    return r;
  }
  LOG(LL_INFO,
      ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s", ctx->cur_boot_cfg_idx,
       cur_cfg.seq, cur_cfg.flags, cur_cfg.app_image_file,
       cur_cfg.app_load_addr, cur_cfg.fs_container_prefix));
  memset(&new_cfg, 0, sizeof(new_cfg));
  new_cfg.seq = cur_cfg.seq - 1;
  new_cfg.flags |= BOOT_F_FIRST_BOOT;
  if (ctx->app_image_file[0] != '\0') {
    strncpy(new_cfg.app_image_file, ctx->app_image_file,
            sizeof(new_cfg.app_image_file));
    new_cfg.app_load_addr = ctx->app_load_addr;
  } else {
    strcpy(new_cfg.app_image_file, cur_cfg.app_image_file);
    new_cfg.app_load_addr = cur_cfg.app_load_addr;
  }
  if (ctx->fs_container_file[0] != '\0') {
    int n = strlen(ctx->fs_container_file);
    do {
      n--;
    } while (ctx->fs_container_file[n] != '.');
    strncpy(new_cfg.fs_container_prefix, ctx->fs_container_file, n);
    new_cfg.flags |= BOOT_F_MERGE_SPIFFS;
  } else {
    strcpy(new_cfg.fs_container_prefix, cur_cfg.fs_container_prefix);
  }
  LOG(LL_INFO,
      ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s", ctx->new_boot_cfg_idx,
       new_cfg.seq, new_cfg.flags, new_cfg.app_image_file,
       new_cfg.app_load_addr, new_cfg.fs_container_prefix));
  r = write_boot_cfg(&new_cfg, ctx->new_boot_cfg_idx);
  if (r < 0) {
    ctx->status_msg = "Could not write new boot cfg";
    return r;
  }
  return 1;
}

void miot_upd_ctx_free(struct miot_upd_ctx *ctx) {
  if (ctx == NULL) return;
  if (ctx->cur_fh >= 0) sl_FsClose(ctx->cur_fh, NULL, NULL, 0);
  if (ctx->cur_fn != NULL) sl_FsDel(ctx->cur_fn, 0);
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

void miot_upd_boot_revert() {
  int boot_cfg_idx = g_boot_cfg_idx;
  struct boot_cfg *cfg = &g_boot_cfg;
  if (!cfg->flags & BOOT_F_FIRST_BOOT) return;
  LOG(LL_ERROR, ("Config %d is bad, reverting", boot_cfg_idx));
  /* Tombstone the current config. */
  cfg->seq = BOOT_CFG_TOMBSTONE_SEQ;
  write_boot_cfg(cfg, boot_cfg_idx);
  miot_system_restart(0);
}

void miot_upd_boot_commit() {
  int boot_cfg_idx = g_boot_cfg_idx;
  struct boot_cfg *cfg = &g_boot_cfg;
  if (!cfg->flags & BOOT_F_FIRST_BOOT) return;
  cfg->flags &= ~(BOOT_F_FIRST_BOOT);
  int r = write_boot_cfg(cfg, boot_cfg_idx);
  if (r < 0) miot_upd_boot_revert();
  LOG(LL_INFO, ("Committed cfg %d, seq 0x%llx", boot_cfg_idx, cfg->seq));
}

int miot_upd_apply_update() {
  int boot_cfg_idx = g_boot_cfg_idx;
  struct boot_cfg *cfg = &g_boot_cfg;
  if (cfg->flags & BOOT_F_MERGE_SPIFFS) {
    int old_boot_cfg_idx = (boot_cfg_idx == 0 ? 1 : 0);
    struct boot_cfg old_boot_cfg;
    int r = read_boot_cfg(old_boot_cfg_idx, &old_boot_cfg);
    if (r < 0) return r;
    struct mount_info old_fs;
    r = fs_mount(old_boot_cfg.fs_container_prefix, &old_fs);
    if (r < 0) return r;
    r = miot_upd_merge_spiffs(&old_fs.fs);
    if (r < 0) return r;
    fs_umount(&old_fs);
    cc3200_fs_flush();
    cfg->flags &= ~(BOOT_F_MERGE_SPIFFS);
  }
  return 0;
}

#endif /* MIOT_ENABLE_UPDATER */
