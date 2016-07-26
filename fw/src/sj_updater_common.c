/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_updater_common.h"

#include <strings.h>

#include "common/cs_crc32.h"
#include "common/spiffs/spiffs.h"

#include "fw/src/sj_console.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_sys_config.h"
#include "fw/src/sj_timers.h"

/*
 * Using static variable (not only c->user_data), it allows to check if update
 * already in progress when another request arrives
 */
struct update_context *s_ctx = NULL;

/* Must be provided externally, usually auto-generated. */
extern const char *build_version;

#define MANIFEST_FILENAME "manifest.json"
#define SHA1SUM_LEN 40

const uint32_t c_zip_file_header_magic = 0x04034b50;
const uint32_t c_zip_cdir_magic = 0x02014b50;
static int s_reboot_timer_id;

enum update_status {
  US_INITED,
  US_WAITING_MANIFEST_HEADER,
  US_WAITING_MANIFEST,
  US_WAITING_FILE_HEADER,
  US_WAITING_FILE,
  US_SKIPPING_DATA,
  US_SKIPPING_DESCRIPTOR,
  US_FINALIZE,
  US_FINISHED,
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct update_context *updater_context_create(enum UPDATE_TYPE ut) {
  if (s_ctx != NULL) {
    CONSOLE_LOG(LL_ERROR, ("Update already in progress"));
    return NULL;
  }

  s_ctx = calloc(1, sizeof(*s_ctx));
  if (s_ctx == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  s_ctx->dev_ctx = sj_upd_ctx_create();
  s_ctx->update_type = ut;

  CONSOLE_LOG(LL_INFO, ("Starting update"));
  return s_ctx;
}

void updater_set_status(struct update_context *ctx, enum update_status st) {
  LOG(LL_DEBUG, ("Update status %d -> %d", (int) ctx->update_status, (int) st));
  ctx->update_status = st;
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
  } else {
    /* No unprocessed, trying to process directly received data */
    ctx->data = data;
    ctx->data_len = len;
  }

  LOG(LL_DEBUG, ("Added %u, size: %u", len, ctx->data_len));
}

static void context_save_unprocessed(struct update_context *ctx) {
  if (ctx->unprocessed.len == 0) {
    mbuf_append(&ctx->unprocessed, ctx->data, ctx->data_len);
    ctx->data = ctx->unprocessed.buf;
    ctx->data_len = ctx->unprocessed.len;
    LOG(LL_DEBUG, ("Added %d bytes to cached data", ctx->data_len));
  }
}

void context_remove_data(struct update_context *ctx, size_t len) {
  if (ctx->unprocessed.len != 0) {
    /* Consumed data from unprocessed*/
    mbuf_remove(&ctx->unprocessed, len);
    ctx->data = ctx->unprocessed.buf;
    ctx->data_len = ctx->unprocessed.len;
  } else {
    /* Consumed received data */
    ctx->data = ctx->data + len;
    ctx->data_len -= len;
  }

  LOG(LL_DEBUG, ("Consumed %u, %u left", len, ctx->data_len));
}

static void context_clear_current_file(struct update_context *ctx) {
  memset(&ctx->current_file, 0, sizeof(ctx->current_file));
}

int is_update_finished(struct update_context *ctx) {
  return ctx->update_status == US_FINISHED;
}

int is_reboot_required(struct update_context *ctx) {
  return ctx->need_reboot;
}

static int parse_zip_file_header(struct update_context *ctx) {
  if (ctx->data_len < ZIP_LOCAL_HDR_SIZE) {
    LOG(LL_DEBUG, ("Zip header is incomplete"));
    /* Need more data*/
    return 0;
  }

  if (memcmp(ctx->data, &c_zip_file_header_magic, 4) != 0) {
    ctx->status_msg = "Malformed archive (invalid file header)";
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
    CONSOLE_LOG(LL_ERROR, ("File is compressed)"));
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

  if (nodir_file_name_len >= sizeof(ctx->current_file.fi.name)) {
    /* We are in charge of file names, right? */
    CONSOLE_LOG(LL_ERROR, ("Too long file name"));
    ctx->status_msg = "Too long file name";
    return -1;
  }
  memcpy(ctx->current_file.fi.name, nodir_file_name, nodir_file_name_len);

  memcpy(&ctx->current_file.fi.size, ctx->data + ZIP_COMPRESSED_SIZE_OFFSET,
         sizeof(ctx->current_file.fi.size));

  uint32_t uncompressed_size;
  memcpy(&uncompressed_size, ctx->data + ZIP_UNCOMPRESSED_SIZE_OFFSET,
         sizeof(uncompressed_size));

  if (ctx->current_file.fi.size != uncompressed_size) {
    /* Probably malformed archive*/
    CONSOLE_LOG(LL_ERROR, ("Malformed archive"));
    ctx->status_msg = "Malformed archive";
    return -1;
  }

  LOG(LL_DEBUG, ("File size: %d", ctx->current_file.fi.size));

  uint16_t gen_flag;
  memcpy(&gen_flag, ctx->data + ZIP_GENFLAG_OFFSET, sizeof(gen_flag));
  ctx->current_file.has_descriptor = gen_flag & (1 << 3);

  LOG(LL_DEBUG, ("General flag=%d", (int) gen_flag));

  memcpy(&ctx->current_file.crc, ctx->data + ZIP_CRC32_OFFSET,
         sizeof(ctx->current_file.crc));

  LOG(LL_DEBUG, ("CRC32: 0x%08x", ctx->current_file.crc));

  context_remove_data(ctx, ZIP_LOCAL_HDR_SIZE + file_name_len + extras_len);

  return 1;
}

static int parse_manifest(struct update_context *ctx) {
  ctx->manifest_data = malloc(ctx->current_file.fi.size);
  if (ctx->manifest_data == NULL) {
    ctx->status_msg = "Out of memory";
    return -1;
  }
  memcpy(ctx->manifest_data, ctx->data, ctx->current_file.fi.size);

  if (json_scanf(ctx->manifest_data, ctx->current_file.fi.size,
                 "{name: %T, platform: %T, version: %T, parts: %T}", &ctx->name,
                 &ctx->platform, &ctx->version, &ctx->parts) <= 0) {
    ctx->status_msg = "Failed to parse manifest";
    return -1;
  }

  if (ctx->platform.len == 0 || ctx->version.len == 0 || ctx->parts.len == 0) {
    ctx->status_msg = "Required manifest field missing";
    return -1;
  }

  CONSOLE_LOG(LL_INFO,
              ("FW: %.*s %.*s %s -> %.*s", (int) ctx->name.len, ctx->name.ptr,
               (int) ctx->platform.len, ctx->platform.ptr, build_version,
               (int) ctx->version.len, ctx->version.ptr));

  context_remove_data(ctx, ctx->current_file.fi.size);

  return 1;
}

static int finalize_write(struct update_context *ctx) {
  if (ctx->current_file.crc != 0 &&
      ctx->current_file.crc != ctx->current_file.crc_current) {
    CONSOLE_LOG(LL_ERROR,
                ("Invalid CRC, want 0x%x, got 0x%x", ctx->current_file.crc,
                 ctx->current_file.crc_current));
    ctx->status_msg = "Invalid CRC";
    return -1;
  }

  int ret = sj_upd_file_end(ctx->dev_ctx, &ctx->current_file.fi);
  if (ret < 0) {
    ctx->status_msg = sj_upd_get_status_msg(ctx->dev_ctx);
    return ret;
  }

  return 1;
}

int updater_process(struct update_context *ctx, const char *data, size_t len) {
  int ret;
  if (len != 0) {
    context_update(ctx, data, len);
  }

  while (true) {
    switch (ctx->update_status) {
      case US_INITED: {
        updater_set_status(ctx, US_WAITING_MANIFEST_HEADER);
      } /* fall through */
      case US_WAITING_MANIFEST_HEADER: {
        if ((ret = parse_zip_file_header(ctx)) <= 0) {
          if (ret == 0) {
            context_save_unprocessed(ctx);
          }
          return ret;
        }
        if (strncmp(ctx->current_file.fi.name, MANIFEST_FILENAME,
                    sizeof(MANIFEST_FILENAME)) != 0) {
          /* We've got file header, but it isn't not metadata */
          CONSOLE_LOG(LL_ERROR, ("Get %s instead of %s",
                                 ctx->current_file.fi.name, MANIFEST_FILENAME));
          return -1;
        }
        updater_set_status(ctx, US_WAITING_MANIFEST);
      } /* fall through */
      case US_WAITING_MANIFEST: {
        /*
         * Assume metadata isn't too big and might be cached
         * otherwise we need streaming json-parser
         */
        if (ctx->data_len < ctx->current_file.fi.size) {
          context_save_unprocessed(ctx);
          return 0;
        }

        if (ctx->current_file.crc != 0 &&
            cs_crc32(0, (const uint8_t *) ctx->data,
                     ctx->current_file.fi.size) != ctx->current_file.crc) {
          ctx->status_msg = "Invalid CRC";
          return -1;
        }

        if ((ret = parse_manifest(ctx)) < 0) return ret;

        if (strncasecmp(ctx->platform.ptr, FW_ARCHITECTURE,
                        strlen(FW_ARCHITECTURE)) != 0) {
          ctx->status_msg = "Wrong platform";
          return -1;
        }

        if (strncmp(ctx->version.ptr, build_version, strlen(build_version)) <=
            0) {
          /* Running the same of higher version */
          if (get_cfg()->update.update_to_any_version == 0) {
            ctx->status_msg = "Device has the same or more recent version";
            CONSOLE_LOG(LL_INFO, (ctx->status_msg));
            return 1; /* Not an error */
          } else {
            CONSOLE_LOG(LL_WARN, ("Same or older version "
                                  "but update_to_any_version is set."));
          }
        }

        if ((ret = sj_upd_begin(ctx->dev_ctx, &ctx->parts,
                                ctx->update_type == utManifest)) < 0) {
          ctx->status_msg = sj_upd_get_status_msg(ctx->dev_ctx);
          CONSOLE_LOG(LL_ERROR, ("Bad manifest: %d %s", ret, ctx->status_msg));
          return ret;
        }

        context_clear_current_file(ctx);
        updater_set_status(ctx, US_WAITING_FILE_HEADER);
      } /* fall through */
      case US_WAITING_FILE_HEADER: {
        if (ctx->data_len < 4) {
          context_save_unprocessed(ctx);
          return 0;
        }
        if (memcmp(ctx->data, &c_zip_cdir_magic, 4) == 0) {
          LOG(LL_DEBUG, ("Reached end of archive, finalizing update"));
          updater_set_status(ctx, US_FINALIZE);
          break;
        }
        if ((ret = parse_zip_file_header(ctx)) <= 0) {
          if (ret == 0) context_save_unprocessed(ctx);
          return ret;
        }

        enum sj_upd_file_action r =
            sj_upd_file_begin(ctx->dev_ctx, &ctx->current_file.fi);

        if (r == SJ_UPDATER_ABORT) {
          ctx->status_msg = sj_upd_get_status_msg(ctx->dev_ctx);
          return -1;
        } else if (r == SJ_UPDATER_SKIP_FILE) {
          updater_set_status(ctx, US_SKIPPING_DATA);
          break;
        }
        updater_set_status(ctx, US_WAITING_FILE);
        ctx->current_file.crc_current = 0;
      } /* fall through */
      case US_WAITING_FILE: {
        struct mg_str to_process;
        to_process.p = ctx->data;
        to_process.len =
            MIN(ctx->current_file.fi.size - ctx->current_file.fi.processed,
                ctx->data_len);

        int num_processed =
            sj_upd_file_data(ctx->dev_ctx, &ctx->current_file.fi, to_process);
        if (num_processed < 0) {
          ctx->status_msg = sj_upd_get_status_msg(ctx->dev_ctx);
          return num_processed;
        } else if (num_processed > 0) {
          ctx->current_file.crc_current =
              cs_crc32(ctx->current_file.crc_current,
                       (const uint8_t *) to_process.p, num_processed);
          context_remove_data(ctx, num_processed);
          ctx->current_file.fi.processed += num_processed;
        }
        LOG(LL_DEBUG,
            ("Processed %d, up to %u, %u left in the buffer", num_processed,
             ctx->current_file.fi.processed, ctx->data_len));

        if (ctx->current_file.fi.processed < ctx->current_file.fi.size) {
          context_save_unprocessed(ctx);
          return 0;
        }

        if (finalize_write(ctx) < 0) {
          return -1;
        }
        context_clear_current_file(ctx);
        updater_set_status(ctx, US_WAITING_FILE_HEADER);
        break;
      }
      case US_SKIPPING_DATA: {
        uint32_t to_skip =
            MIN(ctx->data_len,
                ctx->current_file.fi.size - ctx->current_file.fi.processed);
        ctx->current_file.fi.processed += to_skip;
        LOG(LL_DEBUG, ("Skipping %u bytes, %u total", to_skip,
                       ctx->current_file.fi.processed));
        context_remove_data(ctx, to_skip);

        if (ctx->current_file.fi.processed < ctx->current_file.fi.size) {
          context_save_unprocessed(ctx);
          return 0;
        }

        context_clear_current_file(ctx);
        updater_set_status(ctx, US_SKIPPING_DESCRIPTOR);
      } /* fall through */
      case US_SKIPPING_DESCRIPTOR: {
        int has_descriptor = ctx->current_file.has_descriptor;
        LOG(LL_DEBUG, ("Has descriptor : %d", has_descriptor));
        context_clear_current_file(ctx);
        ctx->current_file.has_descriptor = 0;
        if (has_descriptor) {
          /* If file has descriptor we have to skip 12 bytes after its body */
          ctx->current_file.fi.size = ZIP_FILE_DESCRIPTOR_SIZE;
          updater_set_status(ctx, US_SKIPPING_DATA);
        } else {
          updater_set_status(ctx, US_WAITING_FILE_HEADER);
        }

        context_save_unprocessed(ctx);
        break;
      }
      case US_FINALIZE: {
        if ((ret = sj_upd_finalize(ctx->dev_ctx)) < 0) {
          ctx->status_msg = sj_upd_get_status_msg(ctx->dev_ctx);
          return ret;
        }
        ret = 1;
        ctx->need_reboot = 1;
        ctx->status_msg = "Update applied, finalizing";
        updater_set_status(ctx, US_FINISHED);
      } /* fall through */
      case US_FINISHED: {
        /* After receiving manifest, fw & fs just skipping all data */
        context_remove_data(ctx, ctx->data_len);
        return 1;
      }
    }
  }
}

int updater_finalize(struct update_context *ctx) {
  updater_set_status(ctx, US_FINALIZE);
  return updater_process(ctx, NULL, 0);
}

void updater_finish(struct update_context *ctx) {
  updater_set_status(ctx, US_FINISHED);
}

void updater_context_free(struct update_context *ctx) {
  if (!is_update_finished(ctx)) {
    CONSOLE_LOG(LL_ERROR, ("Update terminated unexpectedly"));
  }
  sj_upd_ctx_free(s_ctx->dev_ctx);
  mbuf_free(&ctx->unprocessed);
  free(ctx->manifest_data);
  free(ctx->base_url);
  free(ctx);
  s_ctx = NULL;
}

static void reboot_timer_cb(void *param) {
#ifdef SJ_ENABLE_CLUBBY
  static int cycles = 0;
  if (sj_console_is_waiting_for_resp() && cycles++ < 100 /* 10 sec */) return;
#endif
  sj_clear_timer(s_reboot_timer_id);
  sj_system_restart(0);
  (void) param;
}

void updater_schedule_reboot(int delay_ms) {
  CONSOLE_LOG(LL_INFO, ("Rebooting in %d ms", delay_ms));
  s_reboot_timer_id = sj_set_c_timer(delay_ms, 1, reboot_timer_cb, NULL);
}

void bin2hex(const uint8_t *src, int src_len, char *dst) {
  int i = 0;
  for (i = 0; i < src_len; i++) {
    sprintf(dst, "%02x", (int) *src);
    dst += 2;
    src += 1;
  }
}

static int file_copy(spiffs *old_fs, char *file_name) {
  int ret = 0;
  FILE *f = NULL;
  spiffs_stat stat;
  int32_t readen, to_read = 0, total = 0;

  CONSOLE_LOG(LL_INFO, ("Copying %s", file_name));

  spiffs_file fd = SPIFFS_open(old_fs, file_name, SPIFFS_RDONLY, 0);
  if (fd < 0) {
    int err = SPIFFS_errno(old_fs);
    if (err == SPIFFS_ERR_NOT_FOUND) {
      CONSOLE_LOG(LL_WARN, ("File %s not found, skipping", file_name));
      return 1;
    } else {
      CONSOLE_LOG(LL_ERROR, ("Failed to open %s, error %d", file_name, err));
      return 0;
    }
  }

  if (SPIFFS_fstat(old_fs, fd, &stat) != SPIFFS_OK) {
    CONSOLE_LOG(LL_ERROR, ("Update failed: cannot get previous %s size (%d)",
                           file_name, SPIFFS_errno(old_fs)));
    goto exit;
  }

  LOG(LL_DEBUG, ("Previous %s size is %d", file_name, stat.size));

  f = fopen(file_name, "w");
  if (f == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to open %s", file_name));
    goto exit;
  }

  char buf[512];
  to_read = MIN(sizeof(buf), stat.size);

  while (to_read != 0) {
    if ((readen = SPIFFS_read(old_fs, fd, buf, to_read)) < 0) {
      CONSOLE_LOG(LL_ERROR, ("Failed to read %d bytes from %s, error %d",
                             to_read, file_name, SPIFFS_errno(old_fs)));
      goto exit;
    }

    if (fwrite(buf, 1, readen, f) != (size_t) readen) {
      CONSOLE_LOG(LL_ERROR,
                  ("Failed to write %d bytes to %s", readen, file_name));
      goto exit;
    }

    total += readen;
    LOG(LL_DEBUG, ("Read: %d, remains: %d", readen, stat.size - total));

    to_read = MIN(sizeof(buf), (stat.size - total));
  }

  LOG(LL_DEBUG, ("Wrote %d to %s", total, file_name));

  ret = 1;

exit:
  if (fd >= 0) SPIFFS_close(old_fs, fd);
  if (f != NULL) fclose(f);

  return ret;
}

int sj_upd_merge_spiffs(spiffs *old_fs) {
  int ret = -1;
  /*
   * here we can use fread & co to read
   * current fs and SPIFFs functions to read
   * old one
   */

  spiffs_DIR dir;
  spiffs_DIR *dir_ptr = SPIFFS_opendir(old_fs, ".", &dir);
  if (dir_ptr == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to open root directory"));
    goto cleanup;
  }

  struct spiffs_dirent de, *de_ptr;
  while ((de_ptr = SPIFFS_readdir(dir_ptr, &de)) != NULL) {
    struct stat st;
    if (stat((const char *) de_ptr->name, &st) != 0) {
      /* File not found on the new fs, copy. */
      if (!file_copy(old_fs, (char *) de_ptr->name)) {
        CONSOLE_LOG(LL_ERROR, ("Failed to copy %s", de_ptr->name));
        goto cleanup;
      }
    }
  }

  ret = 0;

cleanup:
  if (dir_ptr != NULL) SPIFFS_closedir(dir_ptr);

  return ret;
}
