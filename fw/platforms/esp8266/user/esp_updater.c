/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements mg_upd interface.defined in miot_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>
#include <user_interface.h>

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "common/queue.h"
#include "common/spiffs/spiffs.h"
#include "fw/platforms/esp8266/user/esp_fs.h"
#include "fw/src/miot_console.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_updater_rpc.h"
#include "fw/src/miot_updater_hal.h"
#include "fw/src/miot_updater_util.h"

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

enum part_info_type { ptNone, ptBIN, ptFILES };

struct part_info {
  enum part_info_type type;
  uint32_t addr;
  int done;
  int disabled;
  int remote_size;
  uint32 remote_addr;

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

struct miot_upd_ctx {
  struct part_info fw_part;
  struct part_info fs_part;
  struct part_info fs_dir_part;

  int slot_to_write;
  struct part_info *current_part;
  uint32_t current_write_address;
  uint32_t erased_till;
  const char *status_msg;
};

rboot_config *get_rboot_config(void) {
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

uint32_t get_fs_size(uint8_t rom) {
  return get_rboot_config()->fs_sizes[rom];
}

struct miot_upd_ctx *miot_upd_ctx_create(void) {
  return calloc(1, sizeof(struct miot_upd_ctx));
}

const char *miot_upd_get_status_msg(struct miot_upd_ctx *ctx) {
  return ctx->status_msg;
}

static int fill_file_part_info(struct miot_upd_ctx *ctx, struct json_token *tok,
                               const char *part_name, struct part_info *pi) {
  pi->type = ptNone;

  struct json_token sha = JSON_INVALID_TOKEN;
  struct json_token src = JSON_INVALID_TOKEN;

  pi->remote_addr = 0;
  json_scanf(tok->ptr, tok->len, "{addr: %u, cs_sha1: %T, src: %T, size: %d}",
             &pi->remote_addr, &sha, &src, &pi->remote_size);

  if (pi->remote_addr == 0) {
    /* No part found, it might be ok (will be decided later) */
    return -1;
  }

  LOG(LL_DEBUG, ("Addr to write from manifest: %X", pi->addr));
  /*
   * manifest always contain relative addresses, we have to
   * convert them to absolute (+0x100000 for slot #1)
   */
  pi->addr = pi->remote_addr + ctx->slot_to_write * FW_SLOT_SIZE;
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

  pi->type = ptBIN;
  return 1;
}

void fs_dir_parse_cb(void *callback_data, const char *name, size_t name_len,
                     const char *path, const struct json_token *token) {
  struct part_info *pi = (struct part_info *) callback_data;

  (void) name;
  (void) name_len;

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

void bin2hex(const uint8_t *src, int src_len, char *dst);

static int compare_digest(spiffs *fs, const char *file_name,
                          const char *received_digest) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];

  cs_sha1_ctx sha1ctx;
  cs_sha1_init(&sha1ctx);

  spiffs_file file = SPIFFS_open(fs, file_name, SPIFFS_RDONLY, 0);
  if (file < 0) {
    int32_t err_no = SPIFFS_errno(fs);
    if (err_no == SPIFFS_ERR_NOT_FOUND) {
      /* If file is absent on FS, treat this as "should be updated" */
      return 0;
    }
    CONSOLE_LOG(LL_ERROR, ("Failed to open %s (%d)", file_name, err_no));
    return -1;
  }

  int32_t res;
  while ((res = SPIFFS_read(fs, file, read_buf, sizeof(read_buf))) >= 0) {
    cs_sha1_update(&sha1ctx, read_buf, res);
  }

  cs_sha1_final(read_buf, &sha1ctx);

  SPIFFS_close(fs, file);

  bin2hex(read_buf, 20, written_checksum);

  int ret = (strncasecmp(written_checksum, received_digest, SHA1SUM_LEN) == 0);
  if (!ret) {
    LOG(LL_DEBUG, ("%s: on disk %.*s man: %.*s", file_name, SHA1SUM_LEN,
                   written_checksum, SHA1SUM_LEN, received_digest));
  }

  return ret;
}

int verify_checksum(uint32_t addr, size_t len, const char *provided_checksum);

int miot_upd_begin(struct miot_upd_ctx *ctx, struct json_token *parts) {
  const rboot_config *cfg = get_rboot_config();
  struct json_token fs = JSON_INVALID_TOKEN, fw = JSON_INVALID_TOKEN,
                    fs_dir = JSON_INVALID_TOKEN;
  if (cfg == NULL) {
    ctx->status_msg = "Failed to get rBoot config";
    return -1;
  }

  json_scanf(parts->ptr, parts->len, "{fw: %T, fs: %T, fs_dir: %T}", &fw, &fs,
             &fs_dir);

  ctx->slot_to_write = (cfg->current_rom == 0 ? 1 : 0);
  LOG(LL_DEBUG, ("Slot to write: %d", ctx->slot_to_write));

  int fw_part_present =
      (fill_file_part_info(ctx, &fw, "fw", &ctx->fw_part) >= 0);

  if (!fw_part_present) {
    ctx->status_msg = "Firmware part is missing";
    return -1;
  }

  ctx->fw_part.addr =
      ctx->fw_part.remote_addr + ctx->slot_to_write * FW_SLOT_SIZE;

  LOG(LL_DEBUG,
      ("FW addr: 0x%x size: %d", ctx->fw_part.addr, ctx->fw_part.remote_size));

  if (fill_file_part_info(ctx, &fs, "fs", &ctx->fs_part) < 0) {
    ctx->status_msg = "FS part is missing";
    return -1;
  }

  return 1;
}

int verify_checksum(uint32_t addr, size_t len, const char *provided_checksum) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];
  int to_read;

  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);

  size_t len_tmp = len;
  uint32 addr_tmp = addr;
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

    miot_wdt_feed();
  }

  cs_sha1_final(read_buf, &ctx);
  bin2hex(read_buf, 20, written_checksum);
  LOG(LL_DEBUG,
      ("SHA1 %u @ 0x%x = %.*s, want %.*s", len_tmp, addr_tmp, SHA1SUM_LEN,
       written_checksum, SHA1SUM_LEN, provided_checksum));

  if (strncasecmp(written_checksum, provided_checksum, SHA1SUM_LEN) != 0) {
    return -1;
  } else {
    return 1;
  }
}

static int prepare_to_write(struct miot_upd_ctx *ctx,
                            const struct miot_upd_file_info *fi,
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

struct file_info *get_file_info_from_manifest(struct part_info *pi,
                                              const char *current_file_name) {
  struct file_info *fi;
  int dir_len = strlen(pi->files.dir_name);
  SLIST_FOREACH(fi, &pi->files.fhead, entries) {
    /* In zip we have dir_name + file_name */
    if (strncmp(current_file_name, pi->files.dir_name, dir_len) == 0 &&
        current_file_name[dir_len] == '/' &&
        strcmp(current_file_name + dir_len + 1, fi->file_name) == 0) {
      LOG(LL_DEBUG, ("%s should be updated", current_file_name));
      return fi;
    }
    /* in file-by-file update we have filename only */
    if (strcmp(current_file_name, fi->file_name) == 0) {
      LOG(LL_DEBUG, ("%s should be updated", current_file_name));
      return fi;
    }
  }

  return NULL;
}

int miot_upd_get_next_file(struct miot_upd_ctx *ctx, char *buf,
                           size_t buf_size) {
  if (ctx->fw_part.done == 0) {
    if (ctx->fw_part.type == ptNone) {
      CONSOLE_LOG(LL_WARN,
                  ("Fw section not updated because not mentioned in manifest"));
      ctx->fw_part.done = 1;
    } else {
      /* if fw_part must be updated, just send its name like usual one */
      strcpy(buf, ctx->fw_part.fi.file_name);
      return 1;
    }
  };

  if (SLIST_EMPTY(&ctx->fs_dir_part.files.fhead)) {
    return 0; /* All files done */
  }

  struct file_info *fi = SLIST_FIRST(&ctx->fs_dir_part.files.fhead);
  if (strlen(fi->file_name) + strlen(ctx->fs_dir_part.files.dir_name) >
      buf_size) {
    LOG(LL_ERROR, ("File name is too long"));
    return -1;
  }

  int dir_len = strlen(ctx->fs_dir_part.files.dir_name);
  strcpy(buf, ctx->fs_dir_part.files.dir_name);
  buf[dir_len] = '/';
  strcpy(buf + dir_len + 1, fi->file_name);

  LOG(LL_DEBUG, ("File to request: %s", buf));

  return 1;
}

/* 0 - no files to process, 1 - there are files to proceed */
int miot_upd_complete_file_update(struct miot_upd_ctx *ctx,
                                  const char *file_name) {
  struct file_info *fi, *fi_temp;
  SLIST_FOREACH_SAFE(fi, &ctx->fs_dir_part.files.fhead, entries, fi_temp) {
    int dir_len = strlen(ctx->fs_dir_part.files.dir_name);
    if (strncmp(file_name, ctx->fs_dir_part.files.dir_name, dir_len) == 0 &&
        strcmp(file_name + dir_len + 1, fi->file_name) == 0) {
      LOG(LL_DEBUG, ("Removing %s", file_name));
      SLIST_REMOVE(&ctx->fs_dir_part.files.fhead, fi, file_info, entries);
      break;
    }
  }

  return !SLIST_EMPTY(&ctx->fs_dir_part.files.fhead);
}

enum miot_upd_file_action miot_upd_file_begin(
    struct miot_upd_ctx *ctx, const struct miot_upd_file_info *fi) {
  int ret;
  ctx->status_msg = "Failed to update file";
  LOG(LL_DEBUG, ("fi->name=%s", fi->name));
  struct file_info *mfi;
  if (strcmp(fi->name, ctx->fw_part.fi.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fw_part);
  } else if (strcmp(fi->name, ctx->fs_part.fi.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fs_part);
  } else if ((mfi = get_file_info_from_manifest(&ctx->fs_dir_part, fi->name)) !=
             NULL) {
    ctx->current_part = &ctx->fs_dir_part;
    mfi->file = SPIFFS_open(&ctx->fs_dir_part.files.fs, mfi->file_name,
                            SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    if (mfi->file < 0) {
      LOG(LL_ERROR, ("Cannot open file %s (%d)", mfi->file_name,
                     SPIFFS_errno(&ctx->fs_dir_part.files.fs)));
      return MIOT_UPDATER_ABORT;
    }
    ctx->current_part->files.current_file = mfi;

    return MIOT_UPDATER_PROCESS_FILE;
  } else {
    /* We need only fw & fs files, the rest just send to /dev/null */
    return MIOT_UPDATER_SKIP_FILE;
  }
  if (ret < 0) return MIOT_UPDATER_ABORT;
  return (ret == 0 ? MIOT_UPDATER_SKIP_FILE : MIOT_UPDATER_PROCESS_FILE);
}

static int prepare_flash(struct miot_upd_ctx *ctx, uint32_t bytes_to_write) {
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

int miot_upd_file_data(struct miot_upd_ctx *ctx,
                       const struct miot_upd_file_info *fi,
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

int miot_upd_file_end(struct miot_upd_ctx *ctx,
                      const struct miot_upd_file_info *fi) {
  if (ctx->current_part->type == ptFILES) {
    LOG(LL_DEBUG,
        ("File %s updated", ctx->current_part->files.current_file->file_name));
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

int miot_upd_finalize(struct miot_upd_ctx *ctx) {
  if (!ctx->fw_part.done) {
    ctx->status_msg = "Missing fw part";
    return -1;
  }
  if (!ctx->fs_part.done && ctx->fs_dir_part.done == 0) {
    ctx->status_msg = "Missing fs part";
    return -2;
  }

  rboot_config *cfg = get_rboot_config();
  if (ctx->slot_to_write == cfg->current_rom) {
    LOG(LL_INFO, ("Using previous FW"));
    cfg->user_flags = 1;
    rboot_set_config(cfg);
    return 1;
  }

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
  cfg->user_flags = 1;
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

void miot_upd_ctx_free(struct miot_upd_ctx *ctx) {
  free(ctx);
}

int miot_upd_apply_update() {
  rboot_config *cfg = get_rboot_config();
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

  ret = miot_upd_merge_spiffs(&old_fs);

  SPIFFS_unmount(&old_fs);

  return ret;
}

void miot_upd_boot_commit() {
  rboot_config *cfg = get_rboot_config();
  if (!cfg->fw_updated) return;
  LOG(LL_INFO, ("Committing ROM %d", cfg->current_rom));
  cfg->fw_updated = cfg->is_first_boot = 0;
  rboot_set_config(cfg);
}

void miot_upd_boot_revert() {
  rboot_config *cfg = get_rboot_config();
  if (!cfg->fw_updated) return;
  LOG(LL_INFO, ("Update failed, reverting to ROM %d", cfg->previous_rom));
  cfg->current_rom = cfg->previous_rom;
  cfg->fw_updated = cfg->is_first_boot = 0;
  rboot_set_config(cfg);
}
