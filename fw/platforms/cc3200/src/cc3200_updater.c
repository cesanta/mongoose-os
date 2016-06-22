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
#include "fw/src/sj_hal.h"
#include "fw/src/sj_updater_hal.h"
#include "fw/src/sj_updater_util.h"
#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container.h"
#include "fw/platforms/cc3200/src/cc3200_fs_spiffs_container_meta.h"
#include "fw/platforms/cc3200/src/cc3200_updater.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct sj_upd_ctx {
  struct json_token *parts;
  int cur_boot_cfg_idx;
  int new_boot_cfg_idx;
  char app_image_file[MAX_APP_IMAGE_FILE_LEN];
  uint32_t app_load_addr;
  char fs_container_file[MAX_FS_CONTAINER_FNAME_LEN + 3];
  uint32_t fs_size, fs_block_size, fs_page_size, fs_erase_size;
  struct json_token *cur_part;
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
  /* We want to make sure device uses boto loader. */
  ctx->cur_boot_cfg_idx = get_active_boot_cfg_idx();
  if (ctx->cur_boot_cfg_idx < 0) {
    ctx->status_msg = "Could not read current boot cfg";
    return -1;
  }
  ctx->new_boot_cfg_idx = get_inactive_boot_cfg_idx();
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
      DBG(("%.*s -> %.*s", (int) key->len, key->ptr, (int) src_tok->len,
           src_tok->ptr));
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

static long get_num_value(struct json_token *part, const char *key) {
  long result = 0;
  struct json_token *tok = find_json_token(part, key);
  if (tok != NULL && tok->type == JSON_TYPE_NUMBER) {
    result = strtol(tok->ptr, NULL, 0);
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
  cs_sha1_update((cs_sha1_ctx *) arg, data, len);
  return len;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

int verify_checksum(const char *fn, int fs, struct mg_str expected) {
  int r;

  if (expected.len != 40) return -1;

  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);
  if ((r = read_file(fn, 0, fs, sha1_update_cb, &ctx)) < 0) return r;
  _u8 digest[20];
  cs_sha1_final(digest, &ctx);

  char digest_str[41];
  bin2hex(digest, 20, digest_str);

  DBG(("%s: have %.*s, want %.*s", fn, 40, digest_str, 40, expected.p));

  return (strncasecmp(expected.p, digest_str, 40) == 0 ? 1 : 0);
}

/* Create file name by appending ".$idx" to prefix. */
static void create_fname(struct mg_str pfx, int idx, char *fn, int len) {
  int l = MIN(len - 3, pfx.len);
  memcpy(fn, pfx.p, l);
  fn[l++] = '.';
  fn[l++] = ('0' + idx);
  fn[l] = '\0';
}

static int prepare_to_write(struct sj_upd_ctx *ctx,
                            const struct sj_upd_file_info *fi,
                            const char *fname, uint32_t falloc,
                            struct json_token *part) {
  struct mg_str expected_sha1 = get_part_checksum(part);
  if (verify_checksum(fname, fi->size, expected_sha1) > 0) {
    LOG(LL_INFO, ("Digest matched for %s %u (%.*s)", fname, fi->size,
                  (int) expected_sha1.len, expected_sha1.p));
    return 0;
  }
  LOG(LL_INFO, ("Storing %s %u -> %s %u (%.*s)", fi->name, fi->size, fname,
                falloc, (int) expected_sha1.len, expected_sha1.p));
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

enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi) {
  struct mg_str part_name = MG_MK_STR("");
  enum sj_upd_file_action ret = SJ_UPDATER_SKIP_FILE;
  ctx->cur_part = find_part(ctx, fi->name, &part_name);
  if (ctx->cur_part == NULL) return ret;
  /* Drop any indexes from part name, we'll add our own. */
  while (1) {
    char c = part_name.p[part_name.len - 1];
    if (c != '.' && !(c >= '0' && c <= '9')) break;
    part_name.len--;
  }
  struct mg_str type = get_part_type(ctx->cur_part);
  const char *fname = NULL;
  uint32_t falloc = get_num_value(ctx->cur_part, "falloc");
  if (falloc == 0) falloc = fi->size;
  if (mg_vcmp(&type, "app") == 0) {
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
        return SJ_UPDATER_ABORT;
      }
      strncpy(ctx->app_image_file, cur_cfg.app_image_file,
              sizeof(ctx->app_image_file));
    }
#endif
    ctx->app_load_addr = get_num_value(ctx->cur_part, "load_addr");
    if (ctx->app_load_addr >= 0x20000000) {
      fname = ctx->app_image_file;
    } else {
      ctx->status_msg = "Bad/missing app load_addr";
      ret = SJ_UPDATER_ABORT;
    }
  } else if (mg_vcmp(&type, "fs") == 0) {
    ctx->fs_size = get_num_value(ctx->cur_part, "fs_size");
    ctx->fs_block_size = get_num_value(ctx->cur_part, "fs_block_size");
    ctx->fs_page_size = get_num_value(ctx->cur_part, "fs_page_size");
    ctx->fs_erase_size = get_num_value(ctx->cur_part, "fs_erase_size");
    if (ctx->fs_size > 0 && ctx->fs_block_size > 0 && ctx->fs_page_size > 0 &&
        ctx->fs_erase_size > 0) {
      char fs_container_prefix[MAX_FS_CONTAINER_PREFIX_LEN];
      create_fname(part_name, ctx->new_boot_cfg_idx, fs_container_prefix,
                   sizeof(fs_container_prefix));
      /* Delete container 1 so that 0 is the only one. */
      fs_container_fname(fs_container_prefix, 1,
                         (_u8 *) ctx->fs_container_file);
      sl_FsDel((_u8 *) ctx->fs_container_file, 0);
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
      ret = SJ_UPDATER_ABORT;
    }
  }
  if (fname != NULL) {
    int r = prepare_to_write(ctx, fi, fname, falloc, ctx->cur_part);
    if (r < 0) {
      LOG(LL_INFO, ("err = %d", r));
      ret = SJ_UPDATER_ABORT;
    } else {
      ret = (r > 0 ? SJ_UPDATER_PROCESS_FILE : SJ_UPDATER_SKIP_FILE);
    }
  }
  if (ret == SJ_UPDATER_SKIP_FILE) {
    DBG(("Skipping %s %.*s", fi->name, (int) part_name.len, part_name.p));
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
  int r = 1;
  if (ctx->cur_fn == (_u8 *) ctx->fs_container_file) {
    LOG(LL_INFO, ("Writing FS meta: %u %u %u %u", ctx->fs_size,
                  ctx->fs_block_size, ctx->fs_page_size, ctx->fs_erase_size));
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
    r = verify_checksum((const char *) ctx->cur_fn, fi->size,
                        get_part_checksum(ctx->cur_part));
    if (r <= 0) {
      ctx->status_msg = "Checksum mismatch";
      r = -1;
    }
  }
  ctx->cur_fh = -1;
  ctx->cur_fn = NULL;
  return r;
}

int sj_upd_finalize(struct sj_upd_ctx *ctx) {
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

void sj_upd_ctx_free(struct sj_upd_ctx *ctx) {
  if (ctx == NULL) return;
  if (ctx->cur_fh >= 0) sl_FsClose(ctx->cur_fh, NULL, NULL, 0);
  if (ctx->cur_fn != NULL) sl_FsDel(ctx->cur_fn, 0);
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

void revert_update(int boot_cfg_idx, struct boot_cfg *cfg) {
  /* Tombstone the current config. */
  cfg->seq = BOOT_CFG_TOMBSTONE_SEQ;
  write_boot_cfg(cfg, boot_cfg_idx);
  LOG(LL_ERROR, ("Config %d is bad, reverting", boot_cfg_idx));
  sj_system_restart(0);
}

void commit_update(int boot_cfg_idx, struct boot_cfg *cfg) {
  cfg->flags &= ~(BOOT_F_FIRST_BOOT);
  int r = write_boot_cfg(cfg, boot_cfg_idx);
  if (r < 0) revert_update(boot_cfg_idx, cfg);
  LOG(LL_INFO, ("Committed"));
}

int apply_update(int boot_cfg_idx, struct boot_cfg *cfg) {
  if (cfg->flags & BOOT_F_MERGE_SPIFFS) {
    int old_boot_cfg_idx = (boot_cfg_idx == 0 ? 1 : 0);
    struct boot_cfg old_boot_cfg;
    int r = read_boot_cfg(old_boot_cfg_idx, &old_boot_cfg);
    if (r < 0) return r;
    struct mount_info old_fs;
    r = fs_mount(old_boot_cfg.fs_container_prefix, &old_fs);
    if (r < 0) return r;
    /*
     * Delete the inactive old fs container image to free up space
     * for container switch that is likely to happen during merge.
     */
    {
      char fname[MAX_FS_CONTAINER_FNAME_LEN];
      int inactive_idx = (old_fs.cidx == 0 ? 1 : 0);
      fs_container_fname(old_fs.cpfx, inactive_idx, (_u8 *) fname);
      LOG(LL_DEBUG, ("Deleting %s", fname));
      sl_FsDel((const _u8 *) fname, 0);
    }
    r = sj_upd_merge_spiffs(&old_fs.fs);
    if (r < 0) return r;
    fs_umount(&old_fs);
    cfg->flags &= ~(BOOT_F_MERGE_SPIFFS);
  }
  return 1;
}
