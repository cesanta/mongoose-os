/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements sj_upd interface.defined in sj_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>

#include "common/platform.h"
#include "mongoose/mongoose.h"

#include "fw/src/device_config.h"
#include "fw/src/sj_updater_hal.h"
#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct sj_upd_ctx {
  struct json_token *parts;
  int target_boot_cfg;
  char app_file_name[40];
  char fs_file_name[MAX_FS_CONTAINER_PREFIX_LEN + 3];
  const _u8 *cur_fn;
  _i32 cur_fh;
  const char *status_msg;
};

struct sj_upd_ctx *sj_upd_ctx_create() {
  struct sj_upd_ctx *ctx = (struct sj_upd_ctx *) calloc(1, sizeof(*ctx));
  ctx->cur_fh = -1;
  return ctx;
}

const char *sj_upd_get_status_msg(struct sj_upd_ctx *ctx) {
  return ctx->status_msg;
}

int sj_upd_begin(struct sj_upd_ctx *ctx, struct json_token *parts) {
  ctx->parts = parts;
  int active_boot_cfg = get_active_boot_cfg(NULL);
  if (active_boot_cfg < 0) {
    ctx->status_msg = "Could not read current loader cfg";
    return -1;
  }
  ctx->target_boot_cfg = get_inactive_boot_cfg_idx();
  LOG(LL_INFO,
      ("Active boot: %d, target: %d", active_boot_cfg, ctx->target_boot_cfg));
  return 1;
}

static struct json_token *find_part(struct sj_upd_ctx *ctx, const char *src,
                                    struct mg_str *name) {
  struct json_token *t = ctx->parts;
  while (t != NULL && t->type != JSON_TYPE_EOF) {
    t++; /* This points at the key now */
    struct json_token *key = t;
    t++; /* And now to the value */
    struct json_token *src_tok = find_json_token(t, "src");
    if (src_tok != NULL && strncmp(src_tok->ptr, src, src_tok->len) == 0) {
      LOG(LL_DEBUG, ("%.*s -> %.*s", (int) key->len, key->ptr,
                     (int) src_tok->len, src_tok->ptr));
      name->p = key->ptr;
      name->len = key->len;
      return t;
    }
    t += t->num_desc;
  }
  return NULL;
}

static struct mg_str get_str_value(struct json_token *part, const char *key) {
  struct mg_str result = MG_MK_STR("");
  struct json_token *tok = find_json_token(part, key);
  if (tok != NULL && tok->type == JSON_TYPE_STRING) {
    result.p = tok->ptr;
    result.len = tok->len;
  }
  return result;
}

static struct mg_str get_part_type(struct json_token *part) {
  return get_str_value(part, "type");
}

static struct mg_str get_part_checksum(struct json_token *part) {
  return get_str_value(part, "cs_sha1");
}

typedef int (*read_file_cb_t)(_u8 *data, int len, void *arg);
static int read_file(const char *fn, read_file_cb_t cb, void *arg) {
  _i32 fh;
  SlFsFileInfo_t fi;
  _i32 r = sl_FsGetInfo((const _u8 *) fn, 0, &fi);
  if (r < 0) return r;
  r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  if (r < 0) return r;
  _u32 offset = 0;
  while (fi.FileLen > 0) {
    _u8 buf[512];
    _u32 to_read = MIN(fi.FileLen, sizeof(buf));
    r = sl_FsRead(fh, offset, buf, to_read);
    if (r != to_read) break;
    if (cb(buf, to_read, arg) != to_read) break;
    offset += to_read;
    fi.FileLen -= to_read;
  }
  sl_FsClose(fh, NULL, NULL, 0);
  return (fi.FileLen == 0 ? 0 : -1);
}

static int sha1_update_cb(_u8 *data, int len, void *arg) {
  cs_sha1_update((cs_sha1_ctx *) arg, data, len);
  return len;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

int verify_checksum(const char *fn, struct mg_str expected) {
  int r;

  if (expected.len != 40) return -1;

  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);
  if ((r = read_file(fn, sha1_update_cb, &ctx)) < 0) return r;
  _u8 digest[20];
  cs_sha1_final(digest, &ctx);

  char digest_str[40];
  bin2hex(digest, 20, digest_str);

  LOG(LL_DEBUG,
      ("%s: have %.*s, want %.*s", fn, 40, digest_str, 40, expected.p));

  return (strncasecmp(expected.p, digest_str, 40) == 0 ? 1 : -1);
}

/* Create file name by appending ".$idx" to prefix. */
static void create_fname(struct mg_str pfx, int idx, char *fn, int len) {
  int l = MIN(len - 3, pfx.len);
  memcpy(fn, pfx.p, l);
  fn[l++] = '.';
  fn[l++] = ('0' + idx);
  fn[l] = '\0';
}

static int prepare_to_write(struct sj_upd_ctx *ctx, const char *fn,
                            const struct sj_upd_file_info *fi,
                            struct json_token *part) {
  struct mg_str expected_sha1 = get_part_checksum(part);
  if (verify_checksum(fn, expected_sha1) > 0) {
    LOG(LL_INFO, ("Digest matched for %s %u (%.*s)", fn, fi->size,
                  (int) expected_sha1.len, expected_sha1.p));
    return 0;
  }
  LOG(LL_INFO, ("Storing %s %u -> %s (%.*s)", fi->name, fi->size, fn,
                (int) expected_sha1.len, expected_sha1.p));
  ctx->cur_fn = (const _u8 *) fn;
  sl_FsDel(ctx->cur_fn, 0);
  _i32 r = sl_FsOpen(ctx->cur_fn, FS_MODE_OPEN_CREATE(fi->size, 0), NULL,
                     &ctx->cur_fh);
  if (r < 0) {
    ctx->status_msg = "Failed to create file";
    return r;
  }
  return 1;
}

enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi) {
  struct mg_str part_name;
  enum sj_upd_file_action ret = SJ_UPDATER_SKIP_FILE;
  struct json_token *part = find_part(ctx, fi->name, &part_name);
  if (part == NULL) return ret;
  struct mg_str type = get_part_type(part);
  const char *fn = NULL;
  if (mg_vcmp(&type, "app") == 0) {
    create_fname(part_name, ctx->target_boot_cfg, ctx->app_file_name,
                 sizeof(ctx->app_file_name));
    fn = ctx->app_file_name;
  } else if (mg_vcmp(&type, "fs") == 0) {
    char fs_container_prefix[MAX_FS_CONTAINER_PREFIX_LEN];
    create_fname(part_name, ctx->target_boot_cfg, fs_container_prefix,
                 sizeof(fs_container_prefix));
    fs_container_fname(fs_container_prefix, 0, (_u8 *) ctx->fs_file_name);
    fn = ctx->fs_file_name;
  }
  if (fn != NULL) {
    int r = prepare_to_write(ctx, fn, fi, part);
    if (r < 0) {
      ret = SJ_UPDATER_ABORT;
    } else {
      ret = (r > 0 ? SJ_UPDATER_PROCESS_FILE : SJ_UPDATER_SKIP_FILE);
    }
  }
  return ret;
}

int sj_upd_file_data(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi,
                     struct mg_str data) {
  _i32 r = sl_FsWrite(ctx->cur_fh, fi->processed, (_u8 *) data.p, data.len);
  if (r != data.len) {
    ctx->status_msg = "Write failed";
    r = -1;
  }
  return r;
}

int sj_upd_file_end(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi) {
  if (sl_FsClose(ctx->cur_fh, NULL, NULL, 0) != 0) {
    ctx->status_msg = "Close failed";
    ctx->cur_fh = -1;
    return -1;
  }
  ctx->cur_fh = -1;
  ctx->cur_fn = NULL;
  return 1;
}

int sj_upd_finalize(struct sj_upd_ctx *ctx) {
  return 1;
}

void sj_upd_ctx_free(struct sj_upd_ctx *ctx) {
  if (ctx == NULL) return;
  if (ctx->cur_fh >= 0) sl_FsClose(ctx->cur_fh, NULL, NULL, 0);
  if (ctx->cur_fn != NULL) sl_FsDel(ctx->cur_fn, 0);
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}
