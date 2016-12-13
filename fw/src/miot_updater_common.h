/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Common bits of code handling update process.
 * Driven externaly by data source - mg_rpc or POST file upload.
 */

#ifndef CS_FW_SRC_MIOT_UPDATER_COMMON_H_
#define CS_FW_SRC_MIOT_UPDATER_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

#include "mongoose/mongoose.h"
#include "fw/src/miot_timers.h"
#include "fw/src/miot_updater_hal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

extern const uint32_t c_zip_file_header_magic;
extern const uint32_t c_zip_cdir_magic;

struct zip_file_info {
  struct miot_upd_file_info fi;
  uint32_t crc;
  uint32_t crc_current;
  int has_descriptor;
};

struct update_context;
typedef void (*miot_updater_result_cb)(struct update_context *ctx);

struct miot_upd_ctx; /* This struct is defined by HAL and is opaque to us. */
struct update_context {
  int update_status;
  const char *status_msg;

  const char *data;
  size_t data_len;
  struct mbuf unprocessed;
  struct zip_file_info current_file;

  struct json_token name;
  struct json_token platform;
  struct json_token version;
  struct json_token build_id;
  struct json_token parts;

  bool ignore_same_version;
  bool need_reboot;

  int result;
  miot_updater_result_cb result_cb;

  char *manifest_data;
  int file_size;
  char file_name[50];

  struct miot_upd_ctx *dev_ctx;
  miot_timer_id wdt;
  /* Network connection associated with this update, if any.
   * It is only used in case update times out - it is closed. */
  struct mg_connection *nc;

  /*
   * At the end of update this struct is written to a file
   * and then restored after reboot.
   */
  struct update_file_context {
    int64_t id;
    int commit_timeout;
    char mg_rpc_src[100];
  } fctx __attribute__((packed));
};

struct update_context *updater_context_create();
int updater_process(struct update_context *ctx, const char *data, size_t len);
void updater_finish(struct update_context *ctx);
void updater_context_free(struct update_context *ctx);
int updater_finalize(struct update_context *ctx);
int is_write_finished(struct update_context *ctx);
int is_update_finished(struct update_context *ctx);
int is_reboot_required(struct update_context *ctx);

void miot_upd_boot_finish(bool is_successful, bool is_first);
bool miot_upd_commit();
bool miot_upd_revert(bool reboot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_COMMON_H_ */
