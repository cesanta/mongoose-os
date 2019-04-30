/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Updater backend interface.
 * A way to implement custom functionality during OTA.
 * NB: Update can be aborted at any moment, including after finalize
 * (if one of the backends fails to finalize).
 */

#pragma once

#include <stdint.h>

#include "common/mbuf.h"
#include "common/mg_str.h"

#include "mgos_ota.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_UPD_BE_DATA_CHUNK_SIZE
#define MGOS_UPD_BE_DATA_CHUNK_SIZE 512
#endif

struct mgos_ota_be_ctx; /* Defined by backend implementation. */

struct mgos_ota_backend_if {
  /* Create updater context. */
  struct mgos_ota_be_ctx *(*create_ctx)(void);
  /* Get status message. */
  const char *(*get_status_msg)(struct mgos_ota_be_ctx *ctx);
  /*
   * Process the firmware manifest.
   * Manifest info will remain valid for the duration of the update (until
   * free_ctx).
   * If returned value is SKIP, this backend will not get any files and move
   * straight to free_ctx.
   */
  enum mgos_ota_result (*begin)(struct mgos_ota_be_ctx *ctx,
                                const struct mgos_ota_manifest_info *mi);
  /*
   * Begin processing a file from the update package.
   * If returned value is SKIP, there won't be DATA or END calls.
   */
  enum mgos_ota_result (*file_begin)(struct mgos_ota_be_ctx *ctx,
                                     const struct mgos_ota_file_info *fi);

  /*
   * Process a chunk of file data. Data will be delivered to this function in
   * MGOS_UPD_BE_DATA_CHUNK_SIZE chunks.
   * Return number of bytes processed (0 .. data.len)
   * or < 0 for error. In case of error, message should be provided in
   * status_msg.
   */
  int (*file_data)(struct mgos_ota_be_ctx *ctx,
                   const struct mgos_ota_file_info *fi, struct mg_str data);

  /*
   * Finalize a file. Remainder of the data (if any) is passed,
   * number of bytes of that data processed should be returned. The amount of
   * data
   * will be less than MGOS_UPD_BE_DATA_CHUNK_SIZE.
   * Value equal to data.len is an indication of success,
   * < 0 + ctx->status_msg on error.
   */
  int (*file_end)(struct mgos_ota_be_ctx *ctx,
                  const struct mgos_ota_file_info *fi, struct mg_str data);

  /*
   * Finalize the update.
   * Return >= 0 if ok, < 0 + ctx->status_msg on error.
   */
  enum mgos_ota_result (*finalize)(struct mgos_ota_be_ctx *ctx,
                                   bool *need_reboot);

  void (*free_ctx)(struct mgos_ota_be_ctx *ctx);
};

void mgos_ota_register_backend(const struct mgos_ota_backend_if *be_if);

#ifdef __cplusplus
}
#endif
