/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Common bits of code handling update process.
 * Driven externaly by data source - Clubby or POST file upload.
 */

#ifndef CS_FW_SRC_SJ_UPDATER_COMMON_H_
#define CS_FW_SRC_SJ_UPDATER_COMMON_H_

#include <inttypes.h>

#include "fw/src/sj_updater_hal.h"

struct zip_file_info {
  struct sj_upd_file_info fi;
  uint32_t crc;
  uint32_t crc_current;
  int has_descriptor;
};

struct sj_upd_ctx; /* This struct is defined by HAL and is opaque to us. */
struct update_context {
  int update_status;
  const char *status_msg;

  const char *data;
  size_t data_len;
  struct mbuf unprocessed;
  struct zip_file_info current_file;

  struct json_token *manifest;
  struct json_token *name;
  struct json_token *platform;
  struct json_token *version;
  struct json_token *parts;

  int need_reboot;

  int result;
  int archive_size;

  char *manifest_data;

  struct sj_upd_ctx *dev_ctx;
};

struct update_context *updater_context_create();
int updater_process(struct update_context *ctx, const char *data, size_t len);
void updater_finish(struct update_context *ctx);
void updater_context_free(struct update_context *ctx);
void updater_schedule_reboot(int delay_ms);

int is_update_finished(struct update_context *ctx);
int is_reboot_required(struct update_context *ctx);

#endif /* CS_FW_SRC_SJ_UPDATER_COMMON_H_ */
