/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Updater HAL. Devices need to implement this interface.
 */

#ifndef CS_FW_SRC_MIOT_UPDATER_HAL_H_
#define CS_FW_SRC_MIOT_UPDATER_HAL_H_

#include <inttypes.h>

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "frozen/frozen.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct miot_upd_file_info {
  char name[50];
  uint32_t size;
  uint32_t processed;
};

struct miot_upd_ctx *miot_upd_ctx_create(void);

const char *miot_upd_get_status_msg(struct miot_upd_ctx *ctx);

/*
 * Process the firmware manifest. Parsed manifest is available in ctx->manifest,
 * and for convenience the "parts" key within it is in ctx->parts.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int miot_upd_begin(struct miot_upd_ctx *ctx, struct json_token *parts);

/*
 * Decide what to do with the next file.
 * In case of abort, message should be provided in status_msg.
 */
enum miot_upd_file_action {
  MIOT_UPDATER_ABORT,
  MIOT_UPDATER_PROCESS_FILE,
  MIOT_UPDATER_SKIP_FILE,
};
enum miot_upd_file_action miot_upd_file_begin(
    struct miot_upd_ctx *ctx, const struct miot_upd_file_info *fi);

/*
 * Process batch of file data. Return number of bytes processed (0 .. data.len)
 * or < 0 for error. In case of error, message should be provided in status_msg.
 */
int miot_upd_file_data(struct miot_upd_ctx *ctx,
                       const struct miot_upd_file_info *fi, struct mg_str data);

/*
 * Finalize a file.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int miot_upd_file_end(struct miot_upd_ctx *ctx,
                      const struct miot_upd_file_info *fi);

/*
 * Finalize the update.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int miot_upd_finalize(struct miot_upd_ctx *ctx);

void miot_upd_ctx_free(struct miot_upd_ctx *ctx);

int miot_upd_get_next_file(struct miot_upd_ctx *ctx, char *buf,
                           size_t buf_size);

int miot_upd_complete_file_update(struct miot_upd_ctx *ctx,
                                  const char *file_name);

int miot_upd_apply_update();
void miot_upd_boot_commit();
void miot_upd_boot_revert();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_HAL_H_ */
