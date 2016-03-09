/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#include "esp_updater_post.h"

#include <stdint.h>

#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "esp_fs.h"
#include "smartjs/src/sj_v7_ext.h"

#include "smartjs/src/device_config.h"
#include "mongoose/mongoose.h"

#define MANIFEST_FILENAME "manifest.json"
#define FW_SLOT_SIZE 0x100000
#define SHA1SUM_LEN 40

/*
 * --- Zip file local header structure ---
 *                                             size  offset
 * local file header signature   (0x04034b50)   4      0
 * version needed to extract                    2      4
 * general purpose bit flag                     2      6
 * compression method                           2      8
 * last mod file time                           2      10
 * last mod file date                           2      12
 * crc-32                                       4      14
 * compressed size                              4      18
 * uncompressed size                            4      22
 * file name length                             2      26
 * extra field length                           2      28
 * file name (variable size)                    v      30
 * extra field (variable size)                  v
 */

#define ZIP_LOCAL_HDR_SIZE 30U
#define ZIP_GENFLAG_OFFSET 6U
#define ZIP_COMPRESSION_METHOD_OFFSET 8U
#define ZIP_CRC32_OFFSET 14U
#define ZIP_COMPRESSED_SIZE_OFFSET 18U
#define ZIP_UNCOMPRESSED_SIZE_OFFSET 22U
#define ZIP_FILENAME_LEN_OFFSET 26U
#define ZIP_EXTRAS_LEN_OFFSET 28U
#define ZIP_FILENAME_OFFSET 30U
#define ZIP_FILE_DESCRIPTOR_SIZE 12U

static const uint32_t c_zip_header_signature = 0x04034b50;

/* From miniz */
uint32_t mz_crc32(uint32_t crc, const char *ptr, size_t buf_len);

/* Must be provided externally, usually auto-generated. */
extern const char *build_version;

/*
 * Using static variable (not only c->user_data), it allows to check if update
 * already in progress when another request arrives
 */
static struct update_context *s_ctx = NULL;

enum update_status {
  US_INITED,
  US_WAITING_MANIFEST_HEADER,
  US_WAITING_MANIFEST,
  US_WAITING_FILE_HEADER,
  US_WAITING_FILE,
  US_SKIPPING_DATA,
  US_SKIPPING_DESCRIPTOR,
  US_FINISHED
};

struct part_info {
  uint32_t addr;
  char sha1sum[40];
  char file_name[50];
  uint32_t real_size;
};

struct zip_file_info {
  char file_name[50];
  uint32_t file_size;
  uint32_t crc;
  uint32_t crc_current;
  uint32_t file_received_bytes;
  int has_descriptor;
};

struct update_context {
  const char *data;
  size_t data_len;
  struct mbuf unprocessed;
  struct zip_file_info file_info;
  enum update_status update_status;
  const char *status_msg;

  struct part_info fw_part;
  struct part_info fs_part;
  struct part_info *current_part;
  uint32_t current_write_address;
  uint32_t erased_till;

  char version[14];

  int parts_written;

  int slot_to_write;
  int need_reboot;

  int result;
};

static void context_init(struct update_context *ctx) {
  rboot_config cfg = rboot_get_config();
  memset(ctx, 0, sizeof(*ctx));

  ctx->slot_to_write = cfg.current_rom == 0 ? 1 : 0;
  LOG(LL_DEBUG,
      ("Initializing updater, slot to write: %d", ctx->slot_to_write));
}

static void context_release(struct update_context *ctx) {
  mbuf_free(&ctx->unprocessed);
}

/*
 * During its work, updater requires requires to store some data.
 * For example, manifest file, zip header - must be received fully, while
 * content FW/FS files can be flashed directly from recv_mbuf
 * To avoid extra memory usage, context contains plain pointer (*data)
 * and mbuf (unprocessed); data is storing in memory only if where is no way
 * to process it right now.
 */
static void context_update(struct update_context *ctx, const char *data,
                           size_t len) {
  if (ctx->unprocessed.len != 0) {
    /* We have unprocessed data, concatenate them with arrived */
    mbuf_append(&ctx->unprocessed, data, len);
    ctx->data = ctx->unprocessed.buf;
    ctx->data_len = ctx->unprocessed.len;
    LOG(LL_DEBUG, ("Added %u bytes to cached data", len));
  } else {
    /* No unprocessed, trying to process directly received data */
    ctx->data = data;
    ctx->data_len = len;
  }

  LOG(LL_DEBUG, ("Data size: %u bytes", ctx->data_len));
}

static void context_save_unprocessed(struct update_context *ctx) {
  if (ctx->unprocessed.len == 0) {
    mbuf_append(&ctx->unprocessed, ctx->data, ctx->data_len);
    ctx->data = ctx->unprocessed.buf;
    ctx->data_len = ctx->unprocessed.len;
    LOG(LL_DEBUG, ("Added %d bytes to cached data", ctx->data_len));
  } else {
    LOG(LL_DEBUG, ("Skip caching"));
  }
}

static void context_remove_data(struct update_context *ctx, size_t len) {
  if (ctx->unprocessed.len != 0) {
    /* Consumed data from unprocessed*/
    mbuf_remove(&ctx->unprocessed, len);
    ctx->data = ctx->unprocessed.buf;
    ctx->data_len = ctx->unprocessed.len;
    LOG(LL_DEBUG, ("Removed %d bytes from cached data", len));
  } else {
    /* Consumed received data */
    ctx->data = ctx->data + len;
    ctx->data_len -= len;
  }

  LOG(LL_DEBUG, ("Data size: %u bytes", ctx->data_len));
}

static void context_clear_file_info(struct update_context *ctx) {
  memset(&ctx->file_info, 0, sizeof(ctx->file_info));
}

static int fill_zip_header(struct update_context *ctx) {
  if (ctx->data_len < ZIP_LOCAL_HDR_SIZE) {
    LOG(LL_DEBUG, ("Zip header is incomplete"));
    /* Need more data*/
    return 0;
  }

  if (memcmp(ctx->data, &c_zip_header_signature,
             sizeof(c_zip_header_signature)) != 0) {
    ctx->status_msg = "Malformed archive (invalid header)";
    LOG(LL_ERROR, ("Malformed archive (invalid header)"));
    return -1;
  }

  uint16_t file_name_len, extras_len;
  memcpy(&file_name_len, ctx->data + ZIP_FILENAME_LEN_OFFSET,
         sizeof(file_name_len));
  memcpy(&extras_len, ctx->data + ZIP_EXTRAS_LEN_OFFSET, sizeof(extras_len));

  LOG(LL_DEBUG, ("Filename len = %d bytes, extras len = %d bytes",
                 (int) file_name_len, (int) extras_len));
  if (ctx->data_len < ZIP_LOCAL_HDR_SIZE + file_name_len + extras_len) {
    /* Still need mode data */
    return 0;
  }

  uint16_t compression_method;
  memcpy(&compression_method, ctx->data + ZIP_COMPRESSION_METHOD_OFFSET,
         sizeof(compression_method));

  LOG(LL_DEBUG, ("Compression method=%d", (int) compression_method));
  if (compression_method != 0) {
    /* Do not support compressed archives */
    ctx->status_msg = "File is compressed";
    LOG(LL_ERROR, ("File is compressed)"));
    return -1;
  }

  int i;
  char *nodir_file_name = (char *) ctx->data + ZIP_FILENAME_OFFSET;
  uint16_t nodir_file_name_len = file_name_len;
  LOG(LL_DEBUG,
      ("File name: %.*s", (int) nodir_file_name_len, nodir_file_name));

  for (i = 0; i < file_name_len; i++) {
    /* archive may contain folder, but we skip it, using filenames only */
    if (*(ctx->data + ZIP_FILENAME_OFFSET + i) == '/') {
      nodir_file_name = (char *) ctx->data + ZIP_FILENAME_OFFSET + i + 1;
      nodir_file_name_len -= (i + 1);
      break;
    }
  }

  LOG(LL_DEBUG,
      ("File name to use: %.*s", (int) nodir_file_name_len, nodir_file_name));

  if (nodir_file_name_len >= sizeof(ctx->file_info.file_name)) {
    /* We are in charge of file names, right? */
    LOG(LL_ERROR, ("Too long file name"));
    ctx->status_msg = "Too long file name";
    return -1;
  }
  memcpy(ctx->file_info.file_name, nodir_file_name, nodir_file_name_len);

  memcpy(&ctx->file_info.file_size, ctx->data + ZIP_COMPRESSED_SIZE_OFFSET,
         sizeof(ctx->file_info.file_size));

  uint32_t uncompressed_size;
  memcpy(&uncompressed_size, ctx->data + ZIP_UNCOMPRESSED_SIZE_OFFSET,
         sizeof(uncompressed_size));

  if (ctx->file_info.file_size != uncompressed_size) {
    /* Probably malformed archive*/
    LOG(LL_ERROR, ("Malformed archive"));
    ctx->status_msg = "Malformed archive";
    return -1;
  }

  uint16_t gen_flag;
  memcpy(&gen_flag, ctx->data + ZIP_GENFLAG_OFFSET, sizeof(gen_flag));
  ctx->file_info.has_descriptor = gen_flag & (1 << 3);

  LOG(LL_DEBUG, ("General flag=%d", (int) gen_flag));

  memcpy(&ctx->file_info.crc, ctx->data + ZIP_CRC32_OFFSET,
         sizeof(ctx->file_info.crc));

  LOG(LL_DEBUG, ("CRC32: %u", ctx->file_info.crc));

  context_remove_data(ctx, ZIP_LOCAL_HDR_SIZE + file_name_len + extras_len);

  return 1;
}

static int fill_part_info(struct update_context *ctx,
                          struct json_token *parts_tok, const char *part_name,
                          struct part_info *pi) {
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
    LOG(LL_ERROR, ("cs_sha1 token not found in manifest"));
    return -1;
  }

  memcpy(pi->file_name, file_name_tok->ptr, file_name_tok->len);

  LOG(LL_DEBUG,
      ("Part %s : addr: %X sha1: %.*s src: %s", part_name, (int) pi->addr,
       sizeof(pi->sha1sum), pi->sha1sum, pi->file_name));

  return 1;
}

static int fill_manifest(struct update_context *ctx) {
  struct json_token *toks =
      parse_json2((char *) ctx->data, ctx->file_info.file_size);
  if (toks == NULL) {
    LOG(LL_ERROR, ("Failed to parse manifest"));
    goto error;
  }

  struct json_token *parts_tok = find_json_token(toks, "parts");
  if (parts_tok == NULL) {
    LOG(LL_ERROR, ("parts token not found in manifest"));
    goto error;
  }

  if (fill_part_info(ctx, parts_tok, "fw", &ctx->fw_part) < 0 ||
      fill_part_info(ctx, parts_tok, "fs", &ctx->fs_part) < 0) {
    goto error;
  }

  struct json_token *version_tok = find_json_token(toks, "version");
  if (version_tok == NULL || version_tok->type != JSON_TYPE_STRING ||
      version_tok->len != sizeof(ctx->version)) {
    LOG(LL_ERROR, ("version token not found in manifest"));
    goto error;
  }

  memcpy(ctx->version, version_tok->ptr, sizeof(ctx->version));
  LOG(LL_DEBUG, ("Version: %.*s", sizeof(ctx->version), ctx->version));

  context_remove_data(ctx, ctx->file_info.file_size);

  return 1;

error:
  ctx->status_msg = "Invalid manifest";
  return -1;
}

static int prepare_flash(struct update_context *ctx, uint32_t bytes_to_write) {
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

static int process_file_data(struct update_context *ctx, int ignore_data) {
  uint32_t bytes_to_write =
      ctx->file_info.file_size - ctx->file_info.file_received_bytes;
  bytes_to_write =
      bytes_to_write < ctx->data_len ? bytes_to_write : ctx->data_len;

  LOG(LL_DEBUG,
      ("File size: %u, received: %u to_write: %u", ctx->file_info.file_size,
       ctx->file_info.file_received_bytes, bytes_to_write));

  /*
   * if ignore_data=1 we have to skip all received data
   * (usually - extra files in archive)
   * if ignore_data=0 we have to write data to flash
   */
  if (!ignore_data) {
    if (prepare_flash(ctx, bytes_to_write) < 0) {
      ctx->status_msg = "Failed to erase flash";
      return -1;
    }

    /* Write buffer size must be aligned to 4 */
    uint32_t bytes_to_write_aligned = bytes_to_write & -4;
    LOG(LL_DEBUG, ("Aligned size=%u", bytes_to_write_aligned));

    ctx->file_info.crc_current =
        mz_crc32(ctx->file_info.crc_current, ctx->data, bytes_to_write_aligned);

    LOG(LL_DEBUG, ("Writing %u bytes @%X", bytes_to_write_aligned,
                   ctx->current_write_address));

    if (spi_flash_write(ctx->current_write_address, (uint32_t *) ctx->data,
                        bytes_to_write_aligned) != 0) {
      LOG(LL_ERROR, ("Failed to write to flash"));
      ctx->status_msg = "Failed to write to flash";
      return -1;
    }

    ctx->current_write_address += bytes_to_write_aligned;
    ctx->file_info.file_received_bytes += bytes_to_write_aligned;
    context_remove_data(ctx, bytes_to_write_aligned);

    uint32_t rest =
        ctx->file_info.file_size - ctx->file_info.file_received_bytes;

    LOG(LL_DEBUG, ("Rest=%u", rest));
    if (rest != 0 && rest < 4 && ctx->data_len >= 4) {
      /* File size is not aligned to 4, using align buf to write it */
      uint8_t align_buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
      memcpy(align_buf, ctx->data, rest);
      LOG(LL_DEBUG, ("Writing 4 bytes @%X", ctx->current_write_address));
      if (spi_flash_write(ctx->current_write_address, (uint32_t *) align_buf,
                          4) != 0) {
        LOG(LL_ERROR, ("Failed to write to flash"));
        ctx->status_msg = "Failed to write to flash";
        return -1;
      }
      ctx->file_info.crc_current =
          mz_crc32(ctx->file_info.crc_current, ctx->data, rest);

      ctx->file_info.file_received_bytes += rest;
      context_remove_data(ctx, rest);
    }
    if (ctx->file_info.file_received_bytes == ctx->file_info.file_size) {
      LOG(LL_INFO, ("Wrote %s (%u bytes)", ctx->file_info.file_name,
                    ctx->file_info.file_size))
    }
  } else {
    LOG(LL_DEBUG, ("Skipping %u bytes", bytes_to_write));
    ctx->file_info.file_received_bytes += bytes_to_write;
    context_remove_data(ctx, bytes_to_write);
  }

  LOG(LL_DEBUG, ("On exit file size: %u, received: %u",
                 ctx->file_info.file_size, ctx->file_info.file_received_bytes));

  return ctx->file_info.file_size == ctx->file_info.file_received_bytes;
}

static int have_more_files(struct update_context *ctx) {
  LOG(LL_DEBUG, ("Parts written: %d", ctx->parts_written));
  return ctx->parts_written == 2; /* FW + FS */
}

static void bin2hex(const uint8_t *src, int src_len, char *dst) {
  /* TODO(alashkin) : try to use mg_hexdump */
  int i = 0;
  for (i = 0; i < src_len; i++) {
    sprintf(dst, "%02x", (int) *src);
    dst += 2;
    src += 1;
  }
}

static int verify_checksum(uint32_t addr, size_t len,
                           const char *provided_checksum) {
  uint8_t read_buf[4 * 100];
  char written_checksum[50];
  int to_read;

  LOG(LL_DEBUG, ("Verifying checksum"));
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
  LOG(LL_DEBUG,
      ("Written FW checksum: %.*s Provided checksum: %.*s", SHA1SUM_LEN,
       written_checksum, SHA1SUM_LEN, provided_checksum));

  if (strncmp(written_checksum, provided_checksum, SHA1SUM_LEN) != 0) {
    LOG(LL_ERROR, ("Checksum verification failed"));
    return -1;
  } else {
    LOG(LL_DEBUG, ("Checksum verification ok"));
    return 1;
  }
}

static int prepare_to_write(struct update_context *ctx,
                            struct part_info *part) {
  ctx->current_part = part;
  ctx->current_part->real_size = ctx->file_info.file_size;
  ctx->current_write_address = part->addr;
  ctx->erased_till = ctx->current_write_address;

  return 1;
}

static void set_status(struct update_context *ctx, enum update_status st) {
  LOG(LL_DEBUG, ("Update status %d -> %d", (int) ctx->update_status, (int) st));
  ctx->update_status = st;
}

static int finalize_write(struct update_context *ctx) {
  ctx->parts_written++;

  if (ctx->file_info.crc != ctx->file_info.crc_current) {
    LOG(LL_ERROR, ("Invalid CRC, want %u, got %u", ctx->file_info.crc,
                   ctx->file_info.crc_current));
    ctx->status_msg = "Invalid CRC";
    return -1;
  }

  if (verify_checksum(ctx->current_part->addr, ctx->file_info.file_size,
                      ctx->current_part->sha1sum) < 0) {
    ctx->status_msg = "Invalid checksum";
    return -1;
  }

  return 1;
}

void update_rboot_config(struct update_context *ctx) {
  rboot_config cfg = rboot_get_config();
  cfg.previous_rom = cfg.current_rom;
  cfg.current_rom = ctx->slot_to_write;
  cfg.fs_addresses[cfg.current_rom] = ctx->fs_part.addr;
  cfg.fs_sizes[cfg.current_rom] = ctx->fs_part.real_size;
  cfg.roms[cfg.current_rom] = ctx->fw_part.addr;
  cfg.roms_sizes[cfg.current_rom] = ctx->fw_part.real_size;
  cfg.is_first_boot = 1;
  cfg.fw_updated = 1;
  cfg.boot_attempts = 0;
  rboot_set_config(&cfg);

  LOG(LL_DEBUG,
      ("New rboot config: "
       "prev_rom: %d, current_room: %d current_rom addr: %X, "
       "current_rom size: %d, current_fs addr: %X, current_fs size: %d",
       (int) cfg.previous_rom, (int) cfg.current_rom, cfg.roms[cfg.current_rom],
       cfg.roms_sizes[cfg.current_rom], cfg.fs_addresses[cfg.current_rom],
       cfg.fs_sizes[cfg.current_rom]));
}

static int updater_process(struct update_context *ctx, const char *data,
                           size_t len) {
  int ret;
  if (len != 0) {
    context_update(ctx, data, len);
  }

  while (true) {
    switch (ctx->update_status) {
      case US_INITED: {
        set_status(ctx, US_WAITING_MANIFEST_HEADER);
      } /* fall through */
      case US_WAITING_MANIFEST_HEADER: {
        if ((ret = fill_zip_header(ctx)) <= 0) {
          if (ret == 0) {
            context_save_unprocessed(ctx);
          }
          return ret;
        }
        if (strncmp(ctx->file_info.file_name, MANIFEST_FILENAME,
                    sizeof(MANIFEST_FILENAME)) != 0) {
          /* We've got file header, but it isn't not metadata */
          LOG(LL_ERROR, ("Get %s instead of %s", ctx->file_info.file_name,
                         MANIFEST_FILENAME));
          return -1;
        }
        set_status(ctx, US_WAITING_MANIFEST);
      } /* fall through */
      case US_WAITING_MANIFEST: {
        /*
         * Assume metadata isn't too big and might be cached
         * otherwise we need streaming json-parser
         */
        if (ctx->data_len < ctx->file_info.file_size) {
          return 0;
        }

        if (mz_crc32(0, ctx->data, ctx->file_info.file_size) !=
            ctx->file_info.crc) {
          LOG(LL_ERROR, ("Invalid CRC"));
          ctx->status_msg = "Invalid CRC";
          return -1;
        }
        fill_manifest(ctx);
        if (strncmp(ctx->version, build_version, sizeof(ctx->version)) <= 0) {
          /* Running the same of higher version */
          if (get_cfg()->update.update_to_any_version == 0) {
            ctx->status_msg = "Device has the same or more recent version";
            LOG(LL_INFO, (ctx->status_msg));
            return 1; /* Not an error */
          } else {
            LOG(LL_WARN, ("Downgrade, but update to any version enabled"));
          }
        }

        context_clear_file_info(ctx);
        set_status(ctx, US_WAITING_FILE_HEADER);
      } /* fall through */
      case US_WAITING_FILE_HEADER: {
        if ((ret = fill_zip_header(ctx)) <= 0) {
          if (ret == 0) {
            context_save_unprocessed(ctx);
          }
          return ret;
        }

        if (strcmp(ctx->file_info.file_name, ctx->fw_part.file_name) == 0) {
          LOG(LL_DEBUG, ("Initializing FW writer"));
          ret = prepare_to_write(ctx, &ctx->fw_part);
        } else if (strcmp(ctx->file_info.file_name, ctx->fs_part.file_name) ==
                   0) {
          LOG(LL_DEBUG, ("Initializing FS writer"));
          ret = prepare_to_write(ctx, &ctx->fs_part);
        } else {
          /* We need only fw & fs files, the rest just send to /dev/null */
          set_status(ctx, US_SKIPPING_DATA);
          break;
        }

        if (ret < 0) {
          return ret;
        }

        set_status(ctx, US_WAITING_FILE);
      } /* fall through */
      case US_WAITING_FILE: {
        if ((ret = process_file_data(ctx, 0)) <= 0) {
          return ret;
        }

        if (finalize_write(ctx) < 0) {
          return -1;
        }
        context_clear_file_info(ctx);

        ret = have_more_files(ctx);
        LOG(LL_DEBUG, ("More files: %d", ret));

        if (ret > 0) {
          update_rboot_config(ctx);
          ctx->need_reboot = 1;
          ctx->status_msg = "Update completed successfully";
          set_status(ctx, US_FINISHED);
        } else {
          set_status(ctx, US_WAITING_FILE_HEADER);
          break;
        }

        return ret;
      }
      case US_SKIPPING_DATA: {
        if ((ret = process_file_data(ctx, 1)) <= 0) {
          return ret;
        }

        context_clear_file_info(ctx);
        set_status(ctx, US_SKIPPING_DESCRIPTOR);
        break;
      }
      case US_SKIPPING_DESCRIPTOR: {
        int has_descriptor = ctx->file_info.has_descriptor;
        LOG(LL_DEBUG, ("Has descriptor : %d", has_descriptor));
        context_clear_file_info(ctx);
        ctx->file_info.has_descriptor = 0;
        if (has_descriptor) {
          /* If file has descriptor we have to skip 12 bytes after its body */
          ctx->file_info.file_size = ZIP_FILE_DESCRIPTOR_SIZE;
          set_status(ctx, US_SKIPPING_DATA);
        } else {
          set_status(ctx, US_WAITING_FILE_HEADER);
        }

        context_save_unprocessed(ctx);
        break;
      }
      case US_FINISHED: {
        /* After receiving manifest, fw & fs just skipping all data */
        context_remove_data(ctx, ctx->data_len);
        return 1;
      }
    }
  }
  return -1; /* This should never happen, but we have to shut up compiler */
}

static void reboot_timer_cb(void *arg) {
  (void) arg;
  LOG(LL_DEBUG, ("Rebooting"));
  device_reboot();
}

static void schedule_reboot() {
  LOG(LL_DEBUG, ("Scheduling reboot"));
  static os_timer_t reboot_timer;
  os_timer_setfn(&reboot_timer, reboot_timer_cb, NULL);
  os_timer_arm(&reboot_timer, 1000, 0);
}

static int update_finished(struct update_context *ctx) {
  return ctx->update_status == US_FINISHED;
}

static int reboot_requred(struct update_context *ctx) {
  return ctx->need_reboot;
}

static void handle_update_req(struct mg_connection *c, int ev, void *p) {
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      if (s_ctx != 0) {
        mg_printf(c,
                  "HTTP/1.1 400 Bad request\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n"
                  "Update already in progress\r\n");
        LOG(LL_ERROR, ("Update already in progress"));
        c->flags |= MG_F_SEND_AND_CLOSE;
      } else {
        LOG(LL_INFO, ("Updating FW"));
        s_ctx = calloc(1, sizeof(*s_ctx));
        context_init(s_ctx);
        c->user_data = s_ctx;
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      struct update_context *ctx = (struct update_context *) c->user_data;
      struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
      LOG(LL_DEBUG, ("Got %u bytes", mp->data.len))

      /*
       * We can have NULL here if client sends data after completion of
       * update process
       */
      if (ctx != NULL && !update_finished(ctx)) {
        ctx->result = updater_process(ctx, mp->data.p, mp->data.len);
        LOG(LL_DEBUG, ("updater_process res: %d", ctx->result));
        if (ctx->result != 0) {
          set_status(ctx, US_FINISHED);
          /* Don't close connection just yet, not all borowsers like that. */
        }
      }
      break;
    }
    case MG_EV_HTTP_PART_END: {
      struct update_context *ctx = (struct update_context *) c->user_data;
      struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
      LOG(LL_DEBUG, ("MG_EV_HTTP_PART_END: %p %p %d", ctx, mp, mp->status));
      /* Whatever happens, this is the last thing we do. */
      c->flags |= MG_F_SEND_AND_CLOSE;

      if (ctx == NULL) break;
      if (mp->status < 0) {
        /* mp->status < 0 means connection is dead, do not send reply */
        LOG(LL_ERROR, ("Update terminated unexpectedly"));
        break;
      } else {
        if (update_finished(ctx)) {
          mg_printf(c,
                    "HTTP/1.1 %s\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "%s\r\n",
                    ctx->result > 0 ? "200 OK" : "400 Bad request",
                    ctx->status_msg ? ctx->status_msg : "Unknown error");
          LOG(LL_ERROR, ("Update result: %d %s", ctx->result,
                         ctx->status_msg ? ctx->status_msg : "Unknown error"));
          if (reboot_requred(ctx)) {
            LOG(LL_INFO, ("Rebooting device"));
            schedule_reboot();
          }
        } else {
          mg_printf(c,
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "%s\n",
                    "Reached the end without finishing update");
          LOG(LL_ERROR, ("Reached the end without finishing update"));
        }
      }

      context_release(ctx);
      free(ctx);
      s_ctx = NULL;
      c->user_data = NULL;
    } break;
  }
}

void init_updater() {
  device_register_http_endpoint("/update", handle_update_req);
}
