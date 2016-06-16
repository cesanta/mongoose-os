/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 *
 * Implements sj_upd interface.defined in sj_updater_hal.h
 */

#include <inttypes.h>
#include <strings.h>

#include "mongoose/mongoose.h"

#include "fw/src/device_config.h"
#include "fw/src/sj_updater_hal.h"

#define SHA1SUM_LEN 40
#define FW_SLOT_SIZE 0x100000

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct sj_upd_ctx {
  const char *status_msg;
};

struct sj_upd_ctx *sj_upd_ctx_create() {
  return calloc(1, sizeof(struct sj_upd_ctx));
}

const char *sj_upd_get_status_msg(struct sj_upd_ctx *ctx) {
  return ctx->status_msg;
}

int sj_upd_begin(struct sj_upd_ctx *ctx, struct json_token *parts) {
  return 1;
}

enum sj_upd_file_action sj_upd_file_begin(struct sj_upd_ctx *ctx,
                                          const struct sj_upd_file_info *fi) {
  LOG(LL_INFO, ("%s %u", fi->name, fi->size));
  return SJ_UPDATER_SKIP_FILE;
}

int sj_upd_file_data(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi,
                     struct mg_str data) {
  return (int) data.len;
}

int sj_upd_file_end(struct sj_upd_ctx *ctx, const struct sj_upd_file_info *fi) {
  return 1;
}

int sj_upd_finalize(struct sj_upd_ctx *ctx) {
  return 1;
}

void sj_upd_ctx_free(struct sj_upd_ctx *ctx) {
  free(ctx);
}
