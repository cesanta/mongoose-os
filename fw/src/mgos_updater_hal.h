/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Updater HAL. Devices need to implement this interface.
 */

#ifndef CS_FW_SRC_MGOS_UPDATER_HAL_H_
#define CS_FW_SRC_MGOS_UPDATER_HAL_H_

#include <inttypes.h>

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "frozen.h"

#include "mgos_updater.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_upd_hal_ctx *mgos_upd_hal_ctx_create(void);

const char *mgos_upd_get_status_msg(struct mgos_upd_hal_ctx *ctx);

/*
 * Process the firmware manifest. Parsed manifest is available in ctx->manifest,
 * and for convenience the "parts" key within it is in ctx->parts.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int mgos_upd_begin(struct mgos_upd_hal_ctx *ctx, struct json_token *parts);

/*
 * Decide what to do with the next file.
 * In case of abort, message should be provided in status_msg.
 */
enum mgos_upd_file_action {
  MGOS_UPDATER_ABORT,
  MGOS_UPDATER_PROCESS_FILE,
  MGOS_UPDATER_SKIP_FILE,
};
enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_hal_ctx *ctx, const struct mgos_upd_file_info *fi);

/*
 * Process batch of file data. Return number of bytes processed (0 .. data.len)
 * or < 0 for error. In case of error, message should be provided in status_msg.
 */
int mgos_upd_file_data(struct mgos_upd_hal_ctx *ctx,
                       const struct mgos_upd_file_info *fi, struct mg_str data);

/*
 * Finalize a file. Remainder of the data (if any) is passed,
 * number of bytes of that data processed should be returned.
 * Value equal to data.len is an indication of success,
 * < 0 + ctx->status_msg on error.
 */
int mgos_upd_file_end(struct mgos_upd_hal_ctx *ctx,
                      const struct mgos_upd_file_info *fi, struct mg_str data);

/*
 * Finalize the update.
 * Return >= 0 if ok, < 0 + ctx->status_msg on error.
 */
int mgos_upd_finalize(struct mgos_upd_hal_ctx *ctx);

bool mgos_upd_is_first_boot(void);

void mgos_upd_hal_ctx_free(struct mgos_upd_hal_ctx *ctx);

/* Apply update on first boot, usually involves merging filesystem. */
int mgos_upd_apply_update(void);

/*
 * Create a snapshot of currently running firmware (including FS) in
 * a currently inactive slot. There must be no uncommitted update
 * in progress.
 * Returns slot id used for snapshot or < 0 in case of error.
 */
int mgos_upd_create_snapshot(void);

struct mgos_upd_boot_state {
  /* Slot that will be used to load firmware during next boot. */
  int active_slot;
  /* Whether the boot configuration is committed or not.
   * Reboot with uncommitted configration reverts to revert_slot. */
  bool is_committed;
  /* Slot that will be used in case of revert, explicit or implicit. */
  int revert_slot;
};
bool mgos_upd_boot_get_state(struct mgos_upd_boot_state *bs);
bool mgos_upd_boot_set_state(const struct mgos_upd_boot_state *bs);
/* Shortcuts for get and set */
void mgos_upd_boot_commit(void);
void mgos_upd_boot_revert(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_HAL_H_ */
