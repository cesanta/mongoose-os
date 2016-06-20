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
#include "fw/platforms/esp8266/user/esp_fs.h"
#include "fw/src/device_config.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_updater_hal.h"
#include "fw/src/sj_updater_util.h"
#include "fw/platforms/esp8266/user/esp_updater_clubby.h"

#define SHA1SUM_LEN 40
#define FW_SLOT_SIZE 0x100000

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct part_info {
  uint32_t addr;
  char sha1sum[40];
  char file_name[50];
  uint32_t real_size;
  int done;
};

struct sj_upd_ctx {
  struct part_info fw_part;
  struct part_info fs_part;
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

uint8_t get_current_rom() {
  return get_rboot_config()->current_rom;
}

uint32_t get_fw_addr(uint8_t rom) {
  return get_rboot_config()->roms[rom];
}

uint32_t get_fs_addr(uint8_t rom) {
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

static int fill_part_info(struct sj_upd_ctx *ctx, struct json_token *parts_tok,
                          const char *part_name, struct part_info *pi) {
  struct json_token *part_tok = find_json_token(parts_tok, part_name);

  if (part_tok == NULL) {
    LOG(LL_ERROR, ("Part %s not found", part_name));
    return -1;
  }

  struct json_token *addr_tok = find_json_token(part_tok, "addr");
  if (addr_tok == NULL) {
    LOG(LL_ERROR, ("Addr token not found in manifest"));
    return -1;
  }

  /*
   * we can use strtol for non-null terminated string here, we have
   * symbols immediately after address which will  stop number parsing
   */
  pi->addr = strtol(addr_tok->ptr, NULL, 16);
  if (pi->addr == 0) {
    /* Only rboot can has addr = 0, but we do not update rboot now */
    LOG(LL_ERROR, ("Invalid address in manifest"));
    return -1;
  }

  LOG(LL_DEBUG, ("Addr to write from manifest: %X", pi->addr));
  /*
   * manifest always contain relative addresses, we have to
   * convert them to absolute (+0x100000 for slot #1)
   */
  pi->addr += ctx->slot_to_write * FW_SLOT_SIZE;
  LOG(LL_DEBUG, ("Addr to write to use: %X", pi->addr));

  struct json_token *sha1sum_tok = find_json_token(part_tok, "cs_sha1");
  if (sha1sum_tok == NULL || sha1sum_tok->type != JSON_TYPE_STRING ||
      sha1sum_tok->len != sizeof(pi->sha1sum)) {
    LOG(LL_ERROR, ("cs_sha1 token not found in manifest"));
    return -1;
  }
  memcpy(pi->sha1sum, sha1sum_tok->ptr, sizeof(pi->sha1sum));

  struct json_token *file_name_tok = find_json_token(part_tok, "src");
  if (file_name_tok == NULL || file_name_tok->type != JSON_TYPE_STRING ||
      (size_t) file_name_tok->len > sizeof(pi->file_name) - 1) {
    LOG(LL_ERROR, ("src token not found in manifest"));
    return -1;
  }

  memcpy(pi->file_name, file_name_tok->ptr, file_name_tok->len);

  LOG(LL_DEBUG,
      ("Part %s : addr: %X sha1: %.*s src: %s", part_name, (int) pi->addr,
       sizeof(pi->sha1sum), pi->sha1sum, pi->file_name));

  return 1;
}

int sj_upd_begin(struct sj_upd_ctx *ctx, struct json_token *parts) {
  const rboot_config *cfg = get_rboot_config();
  if (cfg == NULL) {
    ctx->status_msg = "Failed to get rBoot config";
    return -1;
  }
  ctx->slot_to_write = (cfg->current_rom == 0 ? 1 : 0);
  LOG(LL_DEBUG, ("Slot to write: %d", ctx->slot_to_write));

  if (fill_part_info(ctx, parts, "fw", &ctx->fw_part) < 0) {
    ctx->status_msg = "Failed to parse fw part";
    return -1;
  }

  if (fill_part_info(ctx, parts, "fs", &ctx->fs_part) < 0) {
    ctx->status_msg = "Failed to parse fs part";
    return -1;
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
      LOG(LL_ERROR, ("Failed to read %d bytes from %X", to_read, addr));
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
  ctx->current_part = part;
  ctx->current_part->real_size = fi->size;
  ctx->current_write_address = part->addr;
  ctx->erased_till = part->addr;
  /* See if current content is the same. */
  if (verify_checksum(part->addr, fi->size, part->sha1sum) == 1) {
    LOG(LL_INFO, ("Digest matched, skipping %s %u @ 0x%x (%.*s)", fi->name,
                  fi->size, part->addr, SHA1SUM_LEN, part->sha1sum));
    part->done = 1;
    return 0;
  }
  LOG(LL_INFO, ("Writing %s %u @ 0x%x (%.*s)", fi->name, fi->size, part->addr,
                SHA1SUM_LEN, part->sha1sum));
  return 1;
}

enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi) {
  int ret;
  if (strcmp(fi->name, ctx->fw_part.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fw_part);
  } else if (strcmp(fi->name, ctx->fs_part.file_name) == 0) {
    ret = prepare_to_write(ctx, fi, &ctx->fs_part);
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
        ctx->current_part->addr + ctx->current_part->real_size >=
            ctx->erased_till + FLASH_ERASE_BLOCK_SIZE) {
      LOG(LL_DEBUG, ("Erasing block @sector %X", sec_no));
      uint32_t block_no = ctx->erased_till / FLASH_ERASE_BLOCK_SIZE;
      if (SPIEraseBlock(block_no) != 0) {
        LOG(LL_ERROR, ("Failed to erase flash block %X", block_no));
        return -1;
      }
      ctx->erased_till = (block_no + 1) * FLASH_ERASE_BLOCK_SIZE;
    } else {
      LOG(LL_DEBUG, ("Erasing sector %X", sec_no));
      if (spi_flash_erase_sector(sec_no) != 0) {
        LOG(LL_ERROR, ("Failed to erase flash sector %X", sec_no));
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
  if (verify_checksum(ctx->current_part->addr, fi->size,
                      ctx->current_part->sha1sum) < 0) {
    ctx->status_msg = "Invalid checksum";
    return -1;
  }
  ctx->current_part->done = 1;
  return 1;
}

static int load_data_from_old_fs(uint32_t old_fs_addr) {
  uint8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
  uint8_t spiffs_fds[32 * 2];
  spiffs old_fs;
  int ret = 0;
  LOG(LL_DEBUG, ("Mounting old FS %d @ 0x%x", FS_SIZE, old_fs_addr));
  if (fs_mount(&old_fs, old_fs_addr, FS_SIZE, spiffs_work_buf, spiffs_fds,
               sizeof(spiffs_fds))) {
    LOG(LL_ERROR, ("Update failed: cannot mount previous file system"));
    return -1;
  }

  ret = sj_upd_merge_spiffs(&old_fs);

  s_clubby_reply = load_clubby_reply(&old_fs);
  /* Do not rollback fw if load_clubby_reply failed */

  SPIFFS_unmount(&old_fs);

  return ret;
}

int finish_update() {
  if (!get_rboot_config()->fw_updated) {
    if (get_rboot_config()->is_first_boot != 0) {
      LOG(LL_INFO, ("Firmware was rolled back, commiting it"));
      get_rboot_config()->is_first_boot = 0;
      rboot_set_config(get_rboot_config());
      s_clubby_upd_status = 1; /* Once we connect wifi we send status 1 */
    }
    return 1;
  }

  /*
   * We merge FS _after_ booting to new FW, to give a chance
   * to change merge behavior in new FW
   */
  uint32_t old_fs_addr = get_fs_addr(get_rboot_config()->previous_rom);
  if (load_data_from_old_fs(old_fs_addr) != 0) {
    /* Ok, commiting update */
    get_rboot_config()->is_first_boot = 0;
    get_rboot_config()->fw_updated = 0;
    rboot_set_config(get_rboot_config());
    LOG(LL_DEBUG, ("Firmware commited"));

    return 1;
  } else {
    LOG(LL_ERROR,
        ("Failed to merge filesystem, rollback to previous firmware"));

    sj_system_restart(0);
    return 0;
  }

  return 1;
}

int sj_upd_finalize(struct sj_upd_ctx *ctx) {
  if (!ctx->fw_part.done) {
    ctx->status_msg = "Missing fw part";
    return -1;
  }
  if (!ctx->fs_part.done) {
    ctx->status_msg = "Missing fs part";
    return -2;
  }

  rboot_config *cfg = get_rboot_config();
  cfg->previous_rom = cfg->current_rom;
  cfg->current_rom = ctx->slot_to_write;
  cfg->fs_addresses[cfg->current_rom] = ctx->fs_part.addr;
  cfg->fs_sizes[cfg->current_rom] = ctx->fs_part.real_size;
  cfg->roms[cfg->current_rom] = ctx->fw_part.addr;
  cfg->roms_sizes[cfg->current_rom] = ctx->fw_part.real_size;
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
