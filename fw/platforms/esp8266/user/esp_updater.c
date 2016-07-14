/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements sj_upd interface.defined in sj_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>
#include <user_interface.h>

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "common/queue.h"
#include "common/spiffs/spiffs.h"
#include "fw/platforms/esp8266/user/esp_fs.h"
#include "fw/src/device_config.h"
#include "fw/src/sj_console.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_updater_clubby.h"
#include "fw/src/sj_updater_hal.h"
#include "fw/src/sj_updater_util.h"

#define SHA1SUM_LEN 40
#define FW_SLOT_SIZE 0x100000
#define UPDATER_MIN_BLOCK_SIZE 2048

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct file_info {
  SLIST_ENTRY(file_info) entries;

  char sha1_sum[40];
  char file_name[50];
  uint32_t size;
  spiffs_file file;
};

enum part_info_type { ptBIN, ptFILES };

struct part_info {
  enum part_info_type type;
  uint32_t addr;
  int done;

  union {
    struct file_info fi;
    struct {
      char dir_name[50];
      SLIST_HEAD(files, file_info) fhead;
      struct file_info *current_file;
      int count;
      uint32_t size;
      spiffs fs;
      uint8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
      uint8_t spiffs_fds[32 * FS_MAX_OPEN_FILES];
    } files;
  };
};

struct sj_upd_ctx {
  struct part_info fw_part;
  struct part_info fs_part;
  struct part_info fs_dir_part;

  int slot_to_write;
  struct part_info *current_part;
  uint32_t current_write_address;
  uint32_t erased_till;
  const char *status_msg;
};

rboot_config *get_rboot_config() {
  static rboot_config *cfg = NULL;
  if (cfg == NULL) {
    cfg = malloc(sizeof(*cfg));
    if (cfg == NULL) {
      LOG(LL_DEBUG, ("Out of memory"));
      return NULL;
    }
    *cfg = rboot_get_config();
  }

  return cfg;
}

static uint8_t get_current_rom() {
  return get_rboot_config()->current_rom;
}

static uint32_t get_fs_addr(uint8_t rom) {
  return get_rboot_config()->fs_addresses[rom];
}

uint32_t get_fs_size(uint8_t rom) {
  return get_rboot_config()->fs_sizes[rom];
}

struct sj_upd_ctx *sj_upd_ctx_create() {
  return calloc(1, sizeof(struct sj_upd_ctx));
}

const char *sj_upd_get_status_msg(struct sj_upd_ctx *ctx) {
  return ctx->status_msg;
}

static int fill_file_part_info(struct sj_upd_ctx *ctx, struct json_token *tok,
                               const char *part_name, struct part_info *pi) {
  pi->type = ptBIN;

  struct json_token sha = JSON_INVALID_TOKEN;
  struct json_token src = JSON_INVALID_TOKEN;

  pi->addr = 0;
  json_scanf(tok->ptr, tok->len, "{addr: %u, cs_sha1: %T, src: %T}", &pi->addr,
             &sha, &src);

  if (pi->addr == 0) {
    /* Only rboot can has addr = 0, but we do not update rboot now */
    CONSOLE_LOG(LL_ERROR, ("Invalid address in manifest"));
    return -1;
  }

  LOG(LL_DEBUG, ("Addr to write from manifest: %X", pi->addr));
  /*
   * manifest always contain relative addresses, we have to
   * convert them to absolute (+0x100000 for slot #1)
   */
  pi->addr += ctx->slot_to_write * FW_SLOT_SIZE;
  LOG(LL_DEBUG, ("Addr to write to use: %X", pi->addr));

  if (sha.len == 0) {
    CONSOLE_LOG(LL_ERROR, ("cs_sha1 token not found in manifest"));
    return -1;
  }
  memcpy(pi->fi.sha1_sum, sha.ptr, sizeof(pi->fi.sha1_sum));

  if (src.len <= 0 || src.len >= (int) sizeof(pi->fi.file_name)) {
    CONSOLE_LOG(LL_ERROR, ("src token not found in manifest"));
    return -1;
  }

  memcpy(pi->fi.file_name, src.ptr, src.len);

  LOG(LL_DEBUG,
      ("Part %s : addr: %X sha1: %.*s src: %s", part_name, (int) pi->addr,
       sizeof(pi->fi.sha1_sum), pi->fi.sha1_sum, pi->fi.file_name));

  return 1;
}

void fs_dir_parse_cb(void *callback_data, const char *path,
                     const struct json_token *token) {
  struct part_info *pi = (struct part_info *) callback_data;

  if (token->type != JSON_TYPE_STRING) {
    /*
     * At this moment we are looking for `cs_sha1`, and
     * token will have type JSON_TYPE_STRING
     */

    return;
  }

  const char sha1_name[] = "cs_sha1";
  int path_len = strlen(path);

  if (path_len < (int) sizeof(sha1_name) - 1) {
    /* Probably, something is wrong with manifest */
    LOG(LL_ERROR, ("Unexpected path: %s", path));
    return;
  }

  if (token->len != SHA1SUM_LEN) {
    LOG(LL_ERROR, ("Malformed sha1"));
    return;
  }
  struct file_info *fi = calloc(1, sizeof(*pi));

  if (fi == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return;
  }

  strncpy(fi->file_name, path + 1 /* skip . */,
          strlen(path) - sizeof(sha1_name) - 1);
  strncpy(fi->sha1_sum, token->ptr, SHA1SUM_LEN);

  LOG(LL_DEBUG, ("Adding file to write: %s (%.*s)", fi->file_name, SHA1SUM_LEN,
                 fi->sha1_sum));

  SLIST_INSERT_HEAD(&pi->files.fhead, fi, entries);

  pi->files.count++;
}

static int fill_dir_part_info(struct sj_upd_ctx *ctx, struct json_token *tok,
                              const char *part_name, struct part_info *pi) {
  (void) ctx;
  pi->type = ptFILES;

  struct json_token src_tok = JSON_INVALID_TOKEN;
  json_scanf(tok->ptr, tok->len, "{src: %T}", &src_tok);

  if (src_tok.type == JSON_TYPE_INVALID) {
    LOG(LL_DEBUG, ("No fs_dir section in this manifest"));
    /*
     * Do not log error here, sections are optional, in general,
     * If this specific section was mandatory it will be reported on
     * higher level
     */
    return -1;
  }

  strcpy(pi->files.dir_name, part_name);

  if (json_walk(src_tok.ptr, src_tok.len, fs_dir_parse_cb, pi) <= 0) {
    LOG(LL_ERROR, ("Malformed manifest"));
    return -1;
  }

  LOG(LL_DEBUG, ("Total files to write to [%s]: %d", pi->files.dir_name,
                 pi->files.count));

  pi->addr = get_fs_addr(ctx->slot_to_write);
  pi->files.size = get_fs_size(ctx->slot_to_write);
  if (pi->files.size == 0) {
    pi->files.size = get_fs_size(get_current_rom());
  }

  LOG(LL_DEBUG,
      ("Addr to write to use: %X size: %d", pi->addr, pi->files.size));

  return 1;
}

int sj_upd_begin(struct sj_upd_ctx *ctx, struct json_token *parts) {
  const rboot_config *cfg = get_rboot_config();
  struct json_token fs = JSON_INVALID_TOKEN, fw = JSON_INVALID_TOKEN,
                    fs_dir = JSON_INVALID_TOKEN;
  if (cfg == NULL) {
    ctx->status_msg = "Failed to get rBoot config";
    return -1;
  }
  ctx->slot_to_write = (cfg->current_rom == 0 ? 1 : 0);
  LOG(LL_DEBUG, ("Slot to write: %d", ctx->slot_to_write));

  json_scanf(parts->ptr, parts->len, "{fw: %T, fs: %T, fs_dir: %T}", &fw, &fs,
             &fs_dir);

  if (fill_file_part_info(ctx, &fw, "fw", &ctx->fw_part) < 0) {
    ctx->status_msg = "Failed to parse fw part";
    return -1;
  }

  int fs_res, fs_dir_result;

  if ((fs_res = fill_file_part_info(ctx, &fs, "fs", &ctx->fs_part)) < 0) {
    ctx->status_msg = "Failed to parse fs part";
  }

  if ((fs_dir_result =
           fill_dir_part_info(ctx, &fs_dir, "fs_dir", &ctx->fs_dir_part)) < 0) {
    ctx->status_msg = "Failed to parse fs_dir part";
  }

  if (fs_res >= 0 && fs_dir_result >= 0) {
    /* TODO(alashkin): make this sutuation an error later */
    LOG(LL_WARN, ("Both fs and fs_dir found, using fs_dir"));
    ctx->fs_part.done = 1;
  }
  return 1;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

int verify_checksum(uint32_t addr, size_t len, const char *provided_checksum) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];
  int to_read;

  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);

  while (len != 0) {
    if (len > sizeof(read_buf)) {
      to_read = sizeof(read_buf);
    } else {
      to_read = len;
    }

    if (spi_flash_read(addr, (uint32_t *) read_buf, to_read) != 0) {
      CONSOLE_LOG(LL_ERROR, ("Failed to read %d bytes from %X", to_read, addr));
      return -1;
    }

    cs_sha1_update(&ctx, read_buf, to_read);
    addr += to_read;
    len -= to_read;

    sj_wdt_feed();
  }

  cs_sha1_final(read_buf, &ctx);
  bin2hex(read_buf, 20, written_checksum);
  LOG(LL_DEBUG, ("SHA1 %u @ 0x%x = %.*s, want %.*s", len, addr, SHA1SUM_LEN,
                 written_checksum, SHA1SUM_LEN, provided_checksum));

  if (strncasecmp(written_checksum, provided_checksum, SHA1SUM_LEN) != 0) {
    return -1;
  } else {
    return 1;
  }
}

static int prepare_to_write(struct sj_upd_ctx *ctx,
                            const struct sj_upd_file_info *fi,
                            struct part_info *part) {
  if (part->done != 0) {
    LOG(LL_DEBUG, ("Skipping %s", fi->name));
    return 0;
  }
  ctx->current_part = part;
  ctx->current_part->fi.size = fi->size;
  ctx->current_write_address = part->addr;
  ctx->erased_till = part->addr;
  /* See if current content is the same. */
  if (verify_checksum(part->addr, fi->size, part->fi.sha1_sum) == 1) {
    CONSOLE_LOG(LL_INFO,
                ("Digest matched, skipping %s %u @ 0x%x (%.*s)", fi->name,
                 fi->size, part->addr, SHA1SUM_LEN, part->fi.sha1_sum));
    part->done = 1;
    return 0;
  }
  CONSOLE_LOG(LL_INFO, ("Writing %s %u @ 0x%x (%.*s)", fi->name, fi->size,
                        part->addr, SHA1SUM_LEN, part->fi.sha1_sum));
  return 1;
}

static int compare_digest(spiffs *fs, const char *file_name,
                          const char *received_digest) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];

  cs_sha1_ctx sha1ctx;
  cs_sha1_init(&sha1ctx);

  spiffs_file file = SPIFFS_open(fs, file_name, SPIFFS_RDONLY, 0);
  if (file < 0) {
    if (SPIFFS_errno(fs)) {
      CONSOLE_LOG(LL_ERROR, ("Failed to open %s", file_name));
      return -1;
    }
  }

  int32_t res;
  while ((res = SPIFFS_read(fs, file, read_buf, sizeof(read_buf))) > 0) {
    cs_sha1_update(&sha1ctx, read_buf, res);
  }

  cs_sha1_final(read_buf, &sha1ctx);

  SPIFFS_close(fs, file);

  bin2hex(read_buf, 20, written_checksum);

  return (strncasecmp(written_checksum, received_digest, SHA1SUM_LEN) == 0);
}

static int prepare_to_update_fs(struct sj_upd_ctx *ctx,
                                struct part_info *part) {
  (void) part;

  LOG(LL_DEBUG,
      ("Trying FS: #%d %d@%X", ctx->slot_to_write,
       get_fs_size(ctx->slot_to_write), get_fs_addr(ctx->slot_to_write)));
  int mount_res =
      fs_mount(&part->files.fs, get_fs_addr(ctx->slot_to_write),
               get_fs_size(ctx->slot_to_write), part->files.spiffs_work_buf,
               part->files.spiffs_fds, sizeof(part->files.spiffs_fds));
  LOG(LL_DEBUG, ("Mount res: %d (%d)", mount_res, SPIFFS_errno));

  if (mount_res != 0 && SPIFFS_errno(&part->files.fs) == SPIFFS_ERR_NOT_A_FS) {
    int32_t res = SPIFFS_format(&part->files.fs);
    if (res != 0) {
      CONSOLE_LOG(LL_ERROR, ("Unable to format filesystem, error: %d (%d)", res,
                             SPIFFS_errno(&part->files.fs)));
      return -1;
    }
    if ((res = fs_mount(&part->files.fs, get_fs_addr(ctx->slot_to_write),
                        get_fs_size(ctx->slot_to_write),
                        part->files.spiffs_work_buf, part->files.spiffs_fds,
                        sizeof(part->files.spiffs_fds))) != 0) {
      CONSOLE_LOG(LL_ERROR, ("Failed to mount filesystem %d(%d)", res,
                             SPIFFS_errno(&part->files.fs)));
      return -1;
    }
  }

  struct file_info *fi, *fi_temp;

  SLIST_FOREACH_SAFE(fi, &part->files.fhead, entries, fi_temp) {
    int dig_res = compare_digest(&part->files.fs, fi->file_name, fi->sha1_sum);

    if (dig_res < 0) {
      CONSOLE_LOG(LL_ERROR, ("FS error"));
      return -1;
    }

    if (dig_res == 1) {
      CONSOLE_LOG(LL_INFO, ("%s is unchanged, skipping", fi->file_name));
      SLIST_REMOVE(&part->files.fhead, fi, file_info, entries);
      part->done++;
    }

    /* 0 means file was changed, do nothing */
  }

  return 1;
}

struct file_info *get_file_info_from_manifest(struct part_info *pi,
                                              const char *current_file_name) {
  struct file_info *fi;
  int dir_len = strlen(pi->files.dir_name);
  SLIST_FOREACH(fi, &pi->files.fhead, entries) {
    if (strncmp(current_file_name, pi->files.dir_name, dir_len) == 0 &&
        current_file_name[dir_len] == '/' &&
        strcmp(current_file_name + dir_len + 1, fi->file_name) == 0) {
      LOG(LL_DEBUG, ("%s should be updated", current_file_name));
      return fi;
    }
  }

  return NULL;
}

enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi) {
  int ret;
  LOG(LL_DEBUG, ("fi->name=%s", fi->name));
  struct file_info *mfi;
  if (strcmp(fi->name, ctx->fw_part.fi.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fw_part);
  } else if (strcmp(fi->name, ctx->fs_part.fi.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fs_part);
  } else if ((mfi = get_file_info_from_manifest(&ctx->fs_dir_part, fi->name)) !=
             NULL) {
    if (ctx->fs_dir_part.done == 0) {
      if (prepare_to_update_fs(ctx, &ctx->fs_dir_part) < 0) {
        return SJ_UPDATER_ABORT;
      }
    }
    ctx->current_part = &ctx->fs_dir_part;
    mfi->file = SPIFFS_open(&ctx->fs_dir_part.files.fs, mfi->file_name,
                            SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (mfi->file < 0) {
      LOG(LL_ERROR, ("Cannot open file %s (%d)", mfi->file_name,
                     SPIFFS_errno(&ctx->fs_dir_part.files.fs)));
      return SJ_UPDATER_ABORT;
    }
    ctx->current_part->files.current_file = mfi;

    ret = SJ_UPDATER_PROCESS_FILE;
  } else {
    /* We need only fw & fs files, the rest just send to /dev/null */
    return SJ_UPDATER_SKIP_FILE;
  }
  if (ret < 0) return SJ_UPDATER_ABORT;
  return (ret == 0 ? SJ_UPDATER_SKIP_FILE : SJ_UPDATER_PROCESS_FILE);
}

static int prepare_flash(struct sj_upd_ctx *ctx, uint32_t bytes_to_write) {
  while (ctx->current_write_address + bytes_to_write > ctx->erased_till) {
    uint32_t sec_no = ctx->erased_till / FLASH_SECTOR_SIZE;

    if ((ctx->erased_till % FLASH_ERASE_BLOCK_SIZE) == 0 &&
        ctx->current_part->addr + ctx->current_part->fi.size >=
            ctx->erased_till + FLASH_ERASE_BLOCK_SIZE) {
      LOG(LL_DEBUG, ("Erasing block @sector %X", sec_no));
      uint32_t block_no = ctx->erased_till / FLASH_ERASE_BLOCK_SIZE;
      if (SPIEraseBlock(block_no) != 0) {
        CONSOLE_LOG(LL_ERROR, ("Failed to erase flash block %X", block_no));
        return -1;
      }
      ctx->erased_till = (block_no + 1) * FLASH_ERASE_BLOCK_SIZE;
    } else {
      LOG(LL_DEBUG, ("Erasing sector %X", sec_no));
      if (spi_flash_erase_sector(sec_no) != 0) {
        CONSOLE_LOG(LL_ERROR, ("Failed to erase flash sector %X", sec_no));
        return -1;
      }
      ctx->erased_till = (sec_no + 1) * FLASH_SECTOR_SIZE;
    }
  }

  return 1;
}

int sj_upd_file_data(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi,
                     struct mg_str data) {
  LOG(LL_DEBUG, ("File size: %u, received: %u to_write: %u", fi->size,
                 fi->processed, data.len));

  if (ctx->current_part->files.current_file != NULL) {
    LOG(LL_DEBUG, ("Processing separated file %s",
                   ctx->current_part->files.current_file->file_name));
    int32_t res = SPIFFS_write(&ctx->fs_dir_part.files.fs,
                               ctx->current_part->files.current_file->file,
                               (void *) data.p, data.len);
    if (res < 0) {
      CONSOLE_LOG(LL_ERROR, ("Failed to write %s",
                             ctx->current_part->files.current_file->file_name));
    } else {
      LOG(LL_DEBUG, ("Writen %d bytes", res));
    }

    return res;
  }
  if (data.len < UPDATER_MIN_BLOCK_SIZE &&
      fi->size - fi->processed > UPDATER_MIN_BLOCK_SIZE) {
    return 0;
  }

  if (prepare_flash(ctx, data.len) < 0) {
    ctx->status_msg = "Failed to erase flash";
    return -1;
  }

  /* Write buffer size must be aligned to 4 */
  int bytes_processed = 0;
  uint32_t bytes_to_write_aligned = data.len & -4;
  if (bytes_to_write_aligned > 0) {
    LOG(LL_DEBUG, ("Writing %u bytes @%X", bytes_to_write_aligned,
                   ctx->current_write_address));

    if (spi_flash_write(ctx->current_write_address, (uint32_t *) data.p,
                        bytes_to_write_aligned) != 0) {
      ctx->status_msg = "Failed to write to flash";
      return -1;
    }
    data.p += bytes_to_write_aligned;
    data.len -= bytes_to_write_aligned;
    ctx->current_write_address += bytes_to_write_aligned;
    bytes_processed += bytes_to_write_aligned;
  }

  const uint32_t rest = fi->size - fi->processed - bytes_to_write_aligned;
  LOG(LL_DEBUG, ("Rest=%u", rest));
  if (rest > 0 && rest < 4 && data.len >= 4) {
    /* File size is not aligned to 4, using align buf to write the tail */
    uint8_t align_buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(align_buf, data.p, rest);
    LOG(LL_DEBUG,
        ("Writing padded %u bytes @%X", rest, ctx->current_write_address));
    if (spi_flash_write(ctx->current_write_address, (uint32_t *) align_buf,
                        4) != 0) {
      ctx->status_msg = "Failed to write to flash";
      return -1;
    }
    bytes_processed += rest;
  }

  return bytes_processed;
}

int sj_upd_file_end(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi) {
  if (ctx->current_part->type == ptFILES) {
    CONSOLE_LOG(LL_DEBUG, ("File %s updated",
                           ctx->current_part->files.current_file->file_name));
    SPIFFS_close(&ctx->current_part->files.fs,
                 ctx->current_part->files.current_file->file);
    if (compare_digest(&ctx->current_part->files.fs,
                       ctx->current_part->files.current_file->file_name,
                       ctx->current_part->files.current_file->sha1_sum) != 1) {
      ctx->status_msg = "Invalid checksum";
      return -1;
    }
    ctx->current_part->done++;
    return 1;
  } else {
    if (verify_checksum(ctx->current_part->addr, fi->size,
                        ctx->current_part->fi.sha1_sum) < 0) {
      ctx->status_msg = "Invalid checksum";
      return -1;
    }
    ctx->current_part->done = 1;
    return 1;
  }
}

int sj_upd_finalize(struct sj_upd_ctx *ctx) {
  if (!ctx->fw_part.done) {
    ctx->status_msg = "Missing fw part";
    return -1;
  }
  if (!ctx->fs_part.done && ctx->fs_dir_part.done == 0) {
    ctx->status_msg = "Missing fs part";
    return -2;
  }

  rboot_config *cfg = get_rboot_config();
  cfg->previous_rom = cfg->current_rom;
  cfg->current_rom = ctx->slot_to_write;
  if (ctx->fs_dir_part.done != 0) {
    /* FS updated by file */
    cfg->fs_addresses[cfg->current_rom] = ctx->fs_dir_part.addr;
    cfg->fs_sizes[cfg->current_rom] = ctx->fs_dir_part.files.size;
  } else {
    cfg->fs_addresses[cfg->current_rom] = ctx->fs_part.addr;
    cfg->fs_sizes[cfg->current_rom] = ctx->fs_part.fi.size;
  }
  cfg->roms[cfg->current_rom] = ctx->fw_part.addr;
  cfg->roms_sizes[cfg->current_rom] = ctx->fw_part.fi.size;
  cfg->is_first_boot = 1;
  cfg->fw_updated = 1;
  cfg->boot_attempts = 0;
  rboot_set_config(cfg);

  LOG(LL_DEBUG,
      ("New rboot config: "
       "prev_rom: %d, current_rom: %d current_rom addr: %X, "
       "current_rom size: %d, current_fs addr: %X, current_fs size: %d",
       (int) cfg->previous_rom, (int) cfg->current_rom,
       cfg->roms[cfg->current_rom], cfg->roms_sizes[cfg->current_rom],
       cfg->fs_addresses[cfg->current_rom], cfg->fs_sizes[cfg->current_rom]));
  return 1;
}

void sj_upd_ctx_free(struct sj_upd_ctx *ctx) {
  free(ctx);
}

int apply_update(rboot_config *cfg) {
  uint8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
  uint8_t spiffs_fds[32 * 2];
  spiffs old_fs;
  int ret = 0;
  uint32_t old_fs_size = cfg->fs_sizes[cfg->previous_rom];
  uint32_t old_fs_addr = cfg->fs_addresses[cfg->previous_rom];
  LOG(LL_INFO, ("Mounting old FS: %d @ 0x%x", old_fs_size, old_fs_addr));
  if (fs_mount(&old_fs, old_fs_addr, old_fs_size, spiffs_work_buf, spiffs_fds,
               sizeof(spiffs_fds))) {
    LOG(LL_ERROR, ("Update failed: cannot mount previous file system"));
    return -1;
  }

  ret = sj_upd_merge_spiffs(&old_fs);

  SPIFFS_unmount(&old_fs);

  return ret;
}

void commit_update(rboot_config *cfg) {
  if (cfg->fw_updated) {
    LOG(LL_INFO, ("Committing ROM %d", cfg->current_rom));
  } else {
    LOG(LL_INFO, ("Reverted to ROM %d", cfg->current_rom));
  }
  cfg->fw_updated = cfg->is_first_boot = 0;
  rboot_set_config(cfg);
}

void revert_update(rboot_config *cfg) {
  LOG(LL_INFO, ("Update failed, reverting to ROM %d", cfg->previous_rom));
  cfg->current_rom = cfg->previous_rom;
  cfg->fw_updated = 0;
  rboot_set_config(cfg);
}
