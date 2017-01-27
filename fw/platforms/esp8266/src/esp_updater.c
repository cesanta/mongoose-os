/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements mg_upd interface defined in mgos_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>
#include <user_interface.h>

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "common/queue.h"
#include "common/spiffs/spiffs.h"
#include "fw/src/mgos_console.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_updater_rpc.h"
#include "fw/src/mgos_updater_hal.h"
#include "fw/src/mgos_updater_util.h"
#include "fw/platforms/esp8266/src/esp_flash_writer.h"
#include "fw/platforms/esp8266/src/esp_fs.h"

#define SHA1SUM_LEN 20
#define FW_SLOT_SIZE 0x100000

struct slot_info {
  int id;
  uint32_t fw_addr;
  uint32_t fw_size;
  uint32_t fs_addr;
  uint32_t fs_size;
};

struct mgos_upd_ctx {
  const char *status_msg;
  struct slot_info write_slot;
  struct json_token fw_file_name, fw_cs_sha1;
  struct json_token fs_file_name, fs_cs_sha1;
  uint32_t fw_size, fs_size;

  struct esp_flash_write_ctx wctx;
  const struct json_token *wcs;
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

static void get_slot_info(int id, struct slot_info *si) {
  memset(si, 0, sizeof(*si));
  si->id = id;
  if (id == 0) {
    si->fw_addr = FW1_ADDR;
    si->fs_addr = FW1_FS_ADDR;
  } else {
    si->fw_addr = FW2_ADDR;
    si->fs_addr = FW2_FS_ADDR;
  }
  si->fw_size = FW_SIZE;
  si->fs_size = FS_SIZE;
}

/*
static void get_active_slot(struct slot_info *si) {
  get_slot_info(get_rboot_config()->current_rom, si);
}
*/

static void get_inactive_slot(struct slot_info *si) {
  int inactive_slot = (get_rboot_config()->current_rom == 0 ? 1 : 0);
  get_slot_info(inactive_slot, si);
}

struct mgos_upd_ctx *mgos_upd_ctx_create(void) {
  return calloc(1, sizeof(struct mgos_upd_ctx));
}

const char *mgos_upd_get_status_msg(struct mgos_upd_ctx *ctx) {
  return ctx->status_msg;
}

int mgos_upd_begin(struct mgos_upd_ctx *ctx, struct json_token *parts) {
  struct json_token fs = JSON_INVALID_TOKEN, fw = JSON_INVALID_TOKEN;
  if (json_scanf(parts->ptr, parts->len, "{fw: %T, fs: %T}", &fw, &fs) != 2) {
    ctx->status_msg = "Invalid manifest";
    return -1;
  }

  if (json_scanf(parts->ptr, parts->len,
                 "{fw: {src: %T, cs_sha1: %T}, fs: {src: %T, cs_sha1: %T}}",
                 &ctx->fw_file_name, &ctx->fw_cs_sha1, &ctx->fs_file_name,
                 &ctx->fs_cs_sha1) != 4) {
    ctx->status_msg = "Incomplete update package";
    return -3;
  }

  if (ctx->fw_cs_sha1.len != SHA1SUM_LEN * 2 ||
      ctx->fs_cs_sha1.len != SHA1SUM_LEN * 2) {
    ctx->status_msg = "Invalid checksum format";
    return -4;
  }

  get_inactive_slot(&ctx->write_slot);

  LOG(LL_INFO, ("FW: %.*s -> 0x%x, FS %.*s -> 0x%x",
                (int) ctx->fw_file_name.len, ctx->fw_file_name.ptr,
                ctx->write_slot.fw_addr, (int) ctx->fs_file_name.len,
                ctx->fs_file_name.ptr, ctx->write_slot.fs_addr));

  return 1;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

static bool verify_checksum(uint32_t addr, size_t len, const char *exp_cs,
                            bool critical) {
  uint32_t read_buf[16];
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

    if (spi_flash_read(addr, read_buf, to_read) != 0) {
      LOG(LL_ERROR, ("Failed to read %d bytes from %X", to_read, addr));
      return false;
    }

    cs_sha1_update(&ctx, (uint8_t *) read_buf, to_read);
    addr += to_read;
    len -= to_read;

    mgos_wdt_feed();
  }

  cs_sha1_final((uint8_t *) read_buf, &ctx);
  char written_checksum[SHA1SUM_LEN * 2 + 1];
  bin2hex((uint8_t *) read_buf, SHA1SUM_LEN, written_checksum);
  bool ret = (strncasecmp(written_checksum, exp_cs, SHA1SUM_LEN * 2) == 0);
  LOG((ret || !critical ? LL_DEBUG : LL_ERROR),
      ("SHA1 %u @ 0x%x = %.*s, want %.*s", len_tmp, addr_tmp, SHA1SUM_LEN * 2,
       written_checksum, SHA1SUM_LEN * 2, exp_cs));
  return ret;
}

enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_ctx *ctx, const struct mgos_upd_file_info *fi) {
  bool res = false;
  struct esp_flash_write_ctx *wctx = &ctx->wctx;
  if (strncmp(fi->name, ctx->fw_file_name.ptr, ctx->fw_file_name.len) == 0) {
    res = esp_init_flash_write_ctx(wctx, ctx->write_slot.fw_addr,
                                   ctx->write_slot.fw_size);
    ctx->wcs = &ctx->fw_cs_sha1;
    ctx->fw_size = fi->size;
  } else if (strncmp(fi->name, ctx->fs_file_name.ptr, ctx->fs_file_name.len) ==
             0) {
    res = esp_init_flash_write_ctx(wctx, ctx->write_slot.fs_addr,
                                   ctx->write_slot.fs_size);
    ctx->wcs = &ctx->fs_cs_sha1;
    ctx->fs_size = fi->size;
  } else {
    LOG(LL_DEBUG, ("Not interesting: %s", fi->name));
    return MGOS_UPDATER_SKIP_FILE;
  }
  if (!res) {
    ctx->status_msg = "Failed to start write";
    return MGOS_UPDATER_ABORT;
  }
  if (fi->size > wctx->max_size) {
    LOG(LL_ERROR, ("Cannot write %s (%u) @ 0x%x: max size %u", fi->name,
                   fi->size, wctx->addr, wctx->max_size));
    ctx->status_msg = "Image too big";
    return MGOS_UPDATER_ABORT;
  }
  wctx->max_size = fi->size;
  if (verify_checksum(wctx->addr, fi->size, ctx->wcs->ptr, false)) {
    LOG(LL_INFO, ("Skip writing %s (%u) @ 0x%x (digest matches)", fi->name,
                  fi->size, wctx->addr));
    return MGOS_UPDATER_SKIP_FILE;
  }
  LOG(LL_INFO,
      ("Start writing %s (%u) @ 0x%x", fi->name, fi->size, wctx->addr));
  return MGOS_UPDATER_PROCESS_FILE;
}

int mgos_upd_file_data(struct mgos_upd_ctx *ctx,
                       const struct mgos_upd_file_info *fi,
                       struct mg_str data) {
  int num_written = esp_flash_write(&ctx->wctx, data);
  if (num_written < 0) {
    ctx->status_msg = "Write failed";
  }
  (void) fi;
  return num_written;
}

int mgos_upd_file_end(struct mgos_upd_ctx *ctx,
                      const struct mgos_upd_file_info *fi, struct mg_str tail) {
  assert(tail.len < 4);
  if (tail.len > 0 && esp_flash_write(&ctx->wctx, tail) != (int) tail.len) {
    ctx->status_msg = "Tail write failed";
    return -1;
  }
  if (!verify_checksum(ctx->wctx.addr, fi->size, ctx->wcs->ptr, true)) {
    ctx->status_msg = "Invalid checksum";
    return -1;
  }
  memset(&ctx->wctx, 0, sizeof(ctx->wctx));
  return tail.len;
}

int mgos_upd_finalize(struct mgos_upd_ctx *ctx) {
  if (ctx->fw_size == 0) {
    ctx->status_msg = "Missing fw part";
    return -1;
  }
  if (ctx->fs_size == 0) {
    ctx->status_msg = "Missing fs part";
    return -2;
  }

  rboot_config *cfg = get_rboot_config();
  cfg->previous_rom = cfg->current_rom;
  cfg->current_rom = ctx->write_slot.id;
  cfg->fs_addresses[cfg->current_rom] = ctx->write_slot.fs_addr;
  cfg->fs_sizes[cfg->current_rom] = ctx->fs_size;
  cfg->roms[cfg->current_rom] = ctx->write_slot.fw_addr;
  cfg->roms_sizes[cfg->current_rom] = ctx->fw_size;
  cfg->is_first_boot = cfg->fw_updated = true;
  cfg->user_flags = 1;
  cfg->boot_attempts = 0;
  rboot_set_config(cfg);

  LOG(LL_INFO,
      ("New rboot config: "
       "prev_rom: %d, current_rom: %d current_rom addr: 0x%x, "
       "current_rom size: %d, current_fs addr: 0x%0x, current_fs size: %d",
       (int) cfg->previous_rom, (int) cfg->current_rom,
       cfg->roms[cfg->current_rom], cfg->roms_sizes[cfg->current_rom],
       cfg->fs_addresses[cfg->current_rom], cfg->fs_sizes[cfg->current_rom]));

  return 1;
}

void mgos_upd_ctx_free(struct mgos_upd_ctx *ctx) {
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

int mgos_upd_apply_update() {
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

  ret = mgos_upd_merge_spiffs(&old_fs);

  SPIFFS_unmount(&old_fs);

  return ret;
}

void mgos_upd_boot_commit() {
  rboot_config *cfg = get_rboot_config();
  if (!cfg->fw_updated) return;
  LOG(LL_INFO, ("Committing ROM %d", cfg->current_rom));
  cfg->fw_updated = cfg->is_first_boot = 0;
  rboot_set_config(cfg);
}

void mgos_upd_boot_revert() {
  rboot_config *cfg = get_rboot_config();
  if (!cfg->fw_updated) return;
  LOG(LL_INFO, ("Update failed, reverting to ROM %d", cfg->previous_rom));
  cfg->current_rom = cfg->previous_rom;
  cfg->fw_updated = cfg->is_first_boot = 0;
  rboot_set_config(cfg);
}
