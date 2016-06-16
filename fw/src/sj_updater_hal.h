/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Updater HAL. Devices need to implement this interface.
 */

#ifndef CS_FW_SRC_SJ_UPDATER_HAL_H_
#define CS_FW_SRC_SJ_UPDATER_HAL_H_

#include <inttypes.h>

#include "common/mbuf.h"
#include "mongoose/mongoose.h" /* TODO(rojer): Pull mg_str out into mg_str.h */

struct sj_upd_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
};

struct sj_upd_ctx *sj_upd_ctx_create();

const char *sj_upd_get_status_msg(struct sj_upd_ctx *ctx);

/*
 * Process the firmware manifest. Parsed manifest is available in ctx->manifest,
 * and for convenience the "parts" key within it is in ctx->parts.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int sj_upd_begin(struct sj_upd_ctx *ctx, struct json_token *parts);

/*
 * Decide what to do with the next file.
 * In case of abort, message should be provided in status_msg.
 */
enum sj_upd_file_action {
  SJ_UPDATER_ABORT,
  SJ_UPDATER_PROCESS_FILE,
  SJ_UPDATER_SKIP_FILE,
};
enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi);

/*
 * Process batch of file data. Return number of bytes processed (0 .. data.len)
 * or < 0 for error. In case of error, message should be provided in status_msg.
 */
int sj_upd_file_data(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi,
                     struct mg_str data);

/*
 * Finalize a file.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int sj_upd_file_end(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi);

/*
 * Finalize the update.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int sj_upd_finalize(struct sj_upd_ctx *ctx);

void sj_upd_ctx_free(struct sj_upd_ctx *ctx);

#endif /* CS_FW_SRC_SJ_UPDATER_HAL_H_ */
