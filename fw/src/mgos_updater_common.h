/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Common bits of code handling update process.
 * Driven externaly by data source - mg_rpc or POST file upload.
 */

#ifndef CS_FW_SRC_MGOS_UPDATER_COMMON_H_
#define CS_FW_SRC_MGOS_UPDATER_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

#include "frozen/frozen.h"
#include "mongoose/mongoose.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_updater.h"
#include "fw/src/mgos_updater_hal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct update_context;
typedef void (*mgos_updater_result_cb)(struct update_context *ctx);

struct mgos_upd_dev_ctx; /* This struct is defined by HAL and is opaque to us.
                            */
struct update_context {
  int update_state;
  const char *status_msg;

  const char *data;
  size_t data_len;
  struct mbuf unprocessed;

  struct mgos_upd_info info;
  uint32_t current_file_crc;
  uint32_t current_file_crc_calc;
  bool current_file_has_descriptor;

  bool ignore_same_version;
  bool need_reboot;

  int result;
  mgos_updater_result_cb result_cb;

  char *manifest_data;
  int file_size;
  char file_name[50];

  struct mgos_upd_dev_ctx *dev_ctx;
  mgos_timer_id wdt;
  /* Network connection associated with this update, if any.
   * It is only used in case update times out - it is closed. */
  struct mg_connection *nc;

  /*
   * At the end of update this struct is written to a file
   * and then restored after reboot.
   */
  struct update_file_context {
    int commit_timeout;
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

void mgos_upd_boot_finish(bool is_successful, bool is_first);
bool mgos_upd_commit();
bool mgos_upd_is_committed();
bool mgos_upd_revert(bool reboot);

int mgos_upd_get_commit_timeout();
bool mgos_upd_set_commit_timeout(int commit_timeout);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_COMMON_H_ */
