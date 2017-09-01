/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * Implements mg_upd interface.defined in mgos_updater_hal.h
 */

#include <stdint.h>
#include <strings.h>

#include "mgos_updater_hal.h"
#include "mgos_updater_util.h"

#if MGOS_ENABLE_UPDATER

struct mgos_upd_hal_ctx {
  const char *status_msg;
};

struct mgos_upd_hal_ctx *mgos_upd_hal_ctx_create(void) {
  struct mgos_upd_hal_ctx *ctx =
      (struct mgos_upd_hal_ctx *) calloc(1, sizeof(*ctx));
  return ctx;
}

const char *mgos_upd_get_status_msg(struct mgos_upd_hal_ctx *ctx) {
  return ctx->status_msg;
}

int mgos_upd_begin(struct mgos_upd_hal_ctx *ctx, struct json_token *parts) {
  (void) parts;
  ctx->status_msg = "Not implemented";
  return -1;
}

enum mgos_upd_file_action mgos_upd_file_begin(
    struct mgos_upd_hal_ctx *ctx, const struct mgos_upd_file_info *fi) {
  (void) fi;
  ctx->status_msg = "Not implemented";
  return MGOS_UPDATER_ABORT;
}

int mgos_upd_file_data(struct mgos_upd_hal_ctx *ctx,
                       const struct mgos_upd_file_info *fi,
                       struct mg_str data) {
  (void) fi;
  (void) data;
  ctx->status_msg = "Not implemented";
  return -1;
}

int mgos_upd_file_end(struct mgos_upd_hal_ctx *ctx,
                      const struct mgos_upd_file_info *fi, struct mg_str tail) {
  (void) fi;
  (void) tail;
  ctx->status_msg = "Not implemented";
  return -1;
}

int mgos_upd_finalize(struct mgos_upd_hal_ctx *ctx) {
  ctx->status_msg = "Not implemented";
  return -1;
}

void mgos_upd_hal_ctx_free(struct mgos_upd_hal_ctx *ctx) {
  free(ctx);
}

int mgos_upd_create_snapshot() {
  return -1;
}

bool mgos_upd_boot_get_state(struct mgos_upd_boot_state *bs) {
  (void) bs;
  return false;
}

bool mgos_upd_boot_set_state(const struct mgos_upd_boot_state *bs) {
  /* TODO(rojer): Implement. */
  (void) bs;
  return false;
}

void mgos_upd_boot_revert() {
}

void mgos_upd_boot_commit() {
}

int mgos_upd_apply_update() {
  return -1;
}

#endif /* MGOS_ENABLE_UPDATER */
