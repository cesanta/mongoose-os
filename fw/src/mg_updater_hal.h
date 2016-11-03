/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Updater HAL. Devices need to implement this interface.
 */

#ifndef CS_FW_SRC_MG_UPDATER_HAL_H_
#define CS_FW_SRC_MG_UPDATER_HAL_H_

#include <inttypes.h>

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "frozen/frozen.h"

struct mg_upd_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
};

struct mg_upd_ctx *mg_upd_ctx_create(void);

const char *mg_upd_get_status_msg(struct mg_upd_ctx *ctx);

/*
 * Process the firmware manifest. Parsed manifest is available in ctx->manifest,
 * and for convenience the "parts" key within it is in ctx->parts.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int mg_upd_begin(struct mg_upd_ctx *ctx, struct json_token *parts);

/*
 * Decide what to do with the next file.
 * In case of abort, message should be provided in status_msg.
 */
enum mg_upd_file_action {
  MG_UPDATER_ABORT,
  MG_UPDATER_PROCESS_FILE,
  MG_UPDATER_SKIP_FILE,
};
enum mg_upd_file_action mg_upd_file_begin(struct mg_upd_ctx *ctx,
                                          const struct mg_upd_file_info *fi);

/*
 * Process batch of file data. Return number of bytes processed (0 .. data.len)
 * or < 0 for error. In case of error, message should be provided in status_msg.
 */
int mg_upd_file_data(struct mg_upd_ctx *ctx, const struct mg_upd_file_info *fi,
                     struct mg_str data);

/*
 * Finalize a file.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int mg_upd_file_end(struct mg_upd_ctx *ctx, const struct mg_upd_file_info *fi);

/*
 * Finalize the update.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int mg_upd_finalize(struct mg_upd_ctx *ctx);

void mg_upd_ctx_free(struct mg_upd_ctx *ctx);

int mg_upd_get_next_file(struct mg_upd_ctx *ctx, char *buf, size_t buf_size);

int mg_upd_complete_file_update(struct mg_upd_ctx *ctx, const char *file_name);

int mg_upd_apply_update();
void mg_upd_boot_commit();
void mg_upd_boot_revert();

#endif /* CS_FW_SRC_MG_UPDATER_HAL_H_ */
