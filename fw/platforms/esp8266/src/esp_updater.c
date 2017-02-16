/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements mg_upd interface defined in mgos_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>

#include <c_types.h>
#include <spi_flash.h>

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

#define CS_LEN 20 /* SHA1 */
#define CS_HEX_LEN (CS_LEN * 2)
#define CS_HEX_BUF_SIZE (CS_HEX_LEN + 1)

#define BOOT_F_MERGE_FS (1U << 0)

struct slot_info {
  int id;
  uint32_t fw_addr;
  uint32_t fw_size;
  uint32_t fw_slot_size;
  uint32_t fs_addr;
  uint32_t fs_size;
  uint32_t fs_slot_size;
};

struct mgos_upd_dev_ctx {
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
  }

  *cfg = rboot_get_config();
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
  rboot_config *cfg = get_rboot_config();
  si->fw_size = cfg->roms_sizes[id];
  si->fw_slot_size = FW_SIZE;
  si->fs_size = cfg->fs_sizes[id];
  si->fs_slot_size = FS_SIZE;
}

struct mgos_upd_dev_ctx *mgos_upd_dev_ctx_create(void) {
  return calloc(1, sizeof(struct mgos_upd_dev_ctx));
}

const char *mgos_upd_get_status_msg(struct mgos_upd_dev_ctx *ctx) {
  return ctx->status_msg;
}

int mgos_upd_begin(struct mgos_upd_dev_ctx *ctx, struct json_token *parts) {
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

  if (ctx->fw_cs_sha1.len != CS_HEX_LEN || ctx->fs_cs_sha1.len != CS_HEX_LEN) {
    ctx->status_msg = "Invalid checksum format";
    return -4;
  }

  struct mgos_upd_boot_state bs;
  if (!mgos_upd_boot_get_state(&bs)) return -5;
  int inactive_slot = (bs.active_slot == 0 ? 1 : 0);
  get_slot_info(inactive_slot, &ctx->write_slot);

  LOG(LL_INFO,
      ("Slot %d, FW: %.*s -> 0x%x, FS %.*s -> 0x%x", ctx->write_slot.id,
       (int) ctx->fw_file_name.len, ctx->fw_file_name.ptr,
       ctx->write_slot.fw_addr, (int) ctx->fs_file_name.len,
       ctx->fs_file_name.ptr, ctx->write_slot.fs_addr));

  return 1;
}

void bin2hex(const uint8_t *src, int src_len, char *dst);

static bool compute_checksum(uint32_t addr, size_t len, char *cs_hex) {
  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);
  while (len != 0) {
    uint32_t read_buf[16];
    uint32_t to_read = sizeof(read_buf);
    if (to_read > len) to_read = len;
    if (spi_flash_read(addr, read_buf, to_read) != 0) {
      LOG(LL_ERROR, ("Failed to read %d bytes from %X", to_read, addr));
      return false;
    }
    cs_sha1_update(&ctx, (uint8_t *) read_buf, to_read);
    mgos_wdt_feed();
    addr += to_read;
    len -= to_read;
  }
  uint8_t cs_buf[CS_LEN];
  cs_sha1_final(cs_buf, &ctx);
  bin2hex(cs_buf, CS_LEN, cs_hex);
  return true;
}

static bool verify_checksum(uint32_t addr, size_t len, const char *exp_cs_hex,
                            bool critical) {
  char cs_hex[CS_HEX_LEN + 1];
  if (!compute_checksum(addr, len, cs_hex)) return false;
  bool ret = (strncasecmp(cs_hex, exp_cs_hex, CS_HEX_LEN) == 0);
  LOG((ret || !critical ? LL_DEBUG : LL_ERROR),
      ("SHA1 %u @ 0x%x = %.*s, want %.*s", len, addr, CS_HEX_LEN, cs_hex,
       CS_HEX_LEN, exp_cs_hex));
  return ret;
}

enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_dev_ctx *ctx, const struct mgos_upd_file_info *fi) {
  bool res = false;
  struct esp_flash_write_ctx *wctx = &ctx->wctx;
  if (strncmp(fi->name, ctx->fw_file_name.ptr, ctx->fw_file_name.len) == 0) {
    res = esp_init_flash_write_ctx(wctx, ctx->write_slot.fw_addr,
                                   ctx->write_slot.fw_slot_size);
    ctx->wcs = &ctx->fw_cs_sha1;
    ctx->fw_size = fi->size;
  } else if (strncmp(fi->name, ctx->fs_file_name.ptr, ctx->fs_file_name.len) ==
             0) {
    res = esp_init_flash_write_ctx(wctx, ctx->write_slot.fs_addr,
                                   ctx->write_slot.fs_slot_size);
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

int mgos_upd_file_data(struct mgos_upd_dev_ctx *ctx,
                       const struct mgos_upd_file_info *fi,
                       struct mg_str data) {
  int num_written = esp_flash_write(&ctx->wctx, data);
  if (num_written < 0) {
    ctx->status_msg = "Write failed";
  }
  (void) fi;
  return num_written;
}

int mgos_upd_file_end(struct mgos_upd_dev_ctx *ctx,
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

int mgos_upd_finalize(struct mgos_upd_dev_ctx *ctx) {
  if (ctx->fw_size == 0) {
    ctx->status_msg = "Missing fw part";
    return -1;
  }
  if (ctx->fs_size == 0) {
    ctx->status_msg = "Missing fs part";
    return -2;
  }

  int slot = ctx->write_slot.id;
  rboot_config *cfg = get_rboot_config();
  cfg->current_rom = slot;
  cfg->previous_rom = (slot == 0 ? 1 : 0);
  cfg->roms[slot] = ctx->write_slot.fw_addr;
  cfg->roms_sizes[slot] = ctx->fw_size;
  cfg->fs_addresses[slot] = ctx->write_slot.fs_addr;
  cfg->fs_sizes[slot] = ctx->fs_size;
  cfg->is_first_boot = cfg->fw_updated = true;
  cfg->boot_attempts = 0;
  cfg->user_flags |= BOOT_F_MERGE_FS;
  if (!rboot_set_config(cfg)) {
    ctx->status_msg = "Failed to set boot config";
    return -3;
  }

  LOG(LL_INFO,
      ("New rboot config: "
       "prev_rom: %d, current_rom: %d current_rom addr: 0x%x, "
       "current_rom size: %d, current_fs addr: 0x%0x, current_fs size: %d",
       (int) cfg->previous_rom, (int) cfg->current_rom,
       cfg->roms[cfg->current_rom], cfg->roms_sizes[cfg->current_rom],
       cfg->fs_addresses[cfg->current_rom], cfg->fs_sizes[cfg->current_rom]));

  return 1;
}

void mgos_upd_dev_ctx_free(struct mgos_upd_dev_ctx *ctx) {
  memset(ctx, 0, sizeof(*ctx));
  free(ctx);
}

int mgos_upd_apply_update() {
  rboot_config *cfg = get_rboot_config();
  if (!cfg->user_flags & BOOT_F_MERGE_FS) return 0;

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

  if (ret == 0) {
    cfg->user_flags &= ~BOOT_F_MERGE_FS;
    rboot_set_config(cfg);
  }

  return ret;
}

static bool copy_region(uint32_t src_addr, uint32_t dst_addr, size_t len) {
  char cs_hex[CS_HEX_LEN + 1];
  if (!compute_checksum(src_addr, len, cs_hex)) return false;
  if (verify_checksum(dst_addr, len, cs_hex, false)) {
    LOG(LL_DEBUG, ("Skip copying %u @ 0x%x -> 0x%x (digest matches)", len,
                   src_addr, dst_addr));
    return true;
  }
  LOG(LL_DEBUG,
      ("Copy %u @ 0x%x -> 0x%x (%s)", len, src_addr, dst_addr, cs_hex));
  struct esp_flash_write_ctx wctx;
  if (!esp_init_flash_write_ctx(&wctx, dst_addr, len)) {
    return false;
  }
  uint32_t offset = 0;
  while (offset < len) {
    uint32_t read_buf[128];
    int to_read = sizeof(read_buf);
    if (offset + to_read > len) to_read = len - offset;
    if (spi_flash_read(src_addr + offset, read_buf, to_read) != 0) {
      LOG(LL_ERROR, ("Failed to read %d @ 0x%x", to_read, src_addr + offset));
      return false;
    }
    int num_written =
        esp_flash_write(&wctx, mg_mk_str_n((const char *) read_buf, to_read));
    if (num_written < 0) return false;
    if (num_written != to_read) {
      /* Flush last chunk */
      int to_write = to_read - num_written;
      num_written = esp_flash_write(
          &wctx,
          mg_mk_str_n(((const char *) read_buf) + num_written, to_write));
      if (num_written != to_write) return false;
    }
    offset += to_read;
    mgos_wdt_feed();
  }
  if (!verify_checksum(dst_addr, len, cs_hex, true)) {
    return false;
  }
  return true;
}

int mgos_upd_create_snapshot() {
  struct slot_info rsi, wsi;
  struct mgos_upd_boot_state bs;
  if (!mgos_upd_boot_get_state(&bs)) return -1;
  int inactive_slot = (bs.active_slot == 0 ? 1 : 0);
  get_slot_info(bs.active_slot, &rsi);
  get_slot_info(inactive_slot, &wsi);
  LOG(LL_INFO, ("Snapshot: %d -> %d, "
                "FW: 0x%x (%u) -> 0x%x, FS: 0x%x (%u) -> 0x%x",
                rsi.id, wsi.id, rsi.fw_addr, rsi.fw_size, wsi.fw_addr,
                rsi.fs_addr, rsi.fs_size, wsi.fs_addr));
  if (!copy_region(rsi.fw_addr, wsi.fw_addr, rsi.fw_size)) return -2;
  if (!copy_region(rsi.fs_addr, wsi.fs_addr, rsi.fs_size)) return -3;
  int slot = wsi.id;
  rboot_config *cfg = get_rboot_config();
  cfg->roms[slot] = wsi.fw_addr;
  cfg->roms_sizes[slot] = rsi.fw_size;
  cfg->fs_addresses[slot] = wsi.fs_addr;
  cfg->fs_sizes[slot] = rsi.fs_size;
  if (!rboot_set_config(cfg)) return -4;
  LOG(LL_INFO, ("Snapshot created"));
  return slot;
}

bool mgos_upd_boot_get_state(struct mgos_upd_boot_state *bs) {
  rboot_config *cfg = get_rboot_config();
  if (cfg == NULL) return false;
  LOG(LL_INFO, ("cur %d prev %d fwu %d", cfg->current_rom, cfg->previous_rom,
                cfg->fw_updated));
  memset(bs, 0, sizeof(*bs));
  bs->active_slot = cfg->current_rom;
  bs->revert_slot = cfg->previous_rom;
  bs->is_committed = !cfg->fw_updated;
  return true;
}

bool mgos_upd_boot_set_state(const struct mgos_upd_boot_state *bs) {
  rboot_config *cfg = get_rboot_config();
  if (cfg == NULL) return false;
  if (bs->active_slot < 0 || bs->active_slot > 1 || bs->revert_slot < 0 ||
      bs->revert_slot > 1) {
    return false;
  }
  cfg->current_rom = bs->active_slot;
  cfg->previous_rom = bs->revert_slot;
  cfg->fw_updated = cfg->is_first_boot = (!bs->is_committed);
  cfg->boot_attempts = 0;
  cfg->user_flags = 0;
  LOG(LL_INFO, ("cur %d prev %d fwu %d", cfg->current_rom, cfg->previous_rom,
                cfg->fw_updated));
  return rboot_set_config(cfg);
}

void mgos_upd_boot_commit() {
  struct mgos_upd_boot_state s;
  if (!mgos_upd_boot_get_state(&s)) return;
  if (s.is_committed) return;
  LOG(LL_INFO, ("Committing ROM %d", s.active_slot));
  s.is_committed = true;
  mgos_upd_boot_set_state(&s);
}

void mgos_upd_boot_revert() {
  struct mgos_upd_boot_state s;
  if (!mgos_upd_boot_get_state(&s)) return;
  if (s.is_committed) return;
  s.active_slot = (s.active_slot == 0 ? 1 : 0);
  LOG(LL_INFO, ("Update failed, reverting to ROM %d", s.active_slot));
  s.is_committed = true;
  mgos_upd_boot_set_state(&s);
}
