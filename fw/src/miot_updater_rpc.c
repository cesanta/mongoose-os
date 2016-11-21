/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_updater_rpc.h"

#include "common/mg_rpc/mg_rpc.h"
#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_console.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_updater_common.h"
#include "fw/src/miot_updater_http.h"
#include "fw/src/miot_utils.h"

#if MIOT_ENABLE_UPDATER_RPC && MIOT_ENABLE_RPC

static struct mg_rpc_request_info *s_update_req;

static void mg_rpc_updater_result(struct update_context *ctx) {
  if (s_update_req == NULL) return;
  if (ctx->need_reboot) {
    /* We're about to reboot, don't reply yet. */
    return;
  }
  mg_rpc_send_errorf(s_update_req, (ctx->result > 0 ? 0 : -1), ctx->status_msg);
  s_update_req = NULL;
}

static void handle_update_req(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  char *blob_url = NULL;
  struct json_token section_tok = JSON_INVALID_TOKEN;
  struct json_token blob_url_tok = JSON_INVALID_TOKEN;
  struct json_token blob_type_tok = JSON_INVALID_TOKEN;
  int commit_timeout = 0;
  struct update_context *ctx = NULL;

  LOG(LL_DEBUG, ("Update request received: %.*s", (int) args.len, args.p));

  const char *reply = "Malformed request";

  if (args.len == 0) {
    goto clean;
  }

  json_scanf(args.p, args.len,
             "{section: %T, blob_url: %T, blob_type: %T, commit_timeout: %d}",
             &section_tok, &blob_url_tok, &blob_type_tok, &commit_timeout);

  /*
   * TODO(alashkin): enable update for another files, not
   * firmware only
   */
  if (section_tok.len == 0 || section_tok.type != JSON_TYPE_STRING ||
      strncmp(section_tok.ptr, "firmware", section_tok.len) != 0 ||
      blob_url_tok.len == 0 || blob_url_tok.type != JSON_TYPE_STRING) {
    goto clean;
  }

  LOG(LL_DEBUG,
      ("Blob url: %.*s blob type: %.*s commit_timeout: %d", blob_url_tok.len,
       blob_url_tok.ptr, blob_type_tok.len, blob_type_tok.ptr, commit_timeout));

  /*
   * If user setup callback for updater, just call it.
   * User can start update with Sys.updater.start()
   */

  blob_url = calloc(1, blob_url_tok.len + 1);
  if (blob_url == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Out of memory"));
    return;
  }

  memcpy(blob_url, blob_url_tok.ptr, blob_url_tok.len);

  ctx = updater_context_create();
  if (ctx == NULL) {
    reply = "Failed to init updater";
    goto clean;
  }
  ctx->fctx.id = ri->id;
  ctx->fctx.commit_timeout = commit_timeout;
  strncpy(ctx->fctx.mg_rpc_src, ri->src.p,
          MIN(ri->src.len, sizeof(ctx->fctx.mg_rpc_src)));
  ctx->result_cb = mg_rpc_updater_result;
  s_update_req = ri;

  miot_updater_http_start(ctx, blob_url);
  free(blob_url);
  return;

clean:
  if (blob_url != NULL) free(blob_url);
  CONSOLE_LOG(LL_ERROR, ("Failed to start update: %s", reply));
  mg_rpc_send_errorf(ri, -1, reply);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void handle_commit_req(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  mg_rpc_send_errorf(ri, miot_upd_commit() ? 0 : -1, NULL);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
  (void) args;
}

static void handle_revert_req(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  bool ok = miot_upd_revert(false /* reboot */);
  mg_rpc_send_errorf(ri, ok ? 0 : -1, NULL);
  ri = NULL;
  if (ok) miot_system_restart_after(100);
  (void) cb_arg;
  (void) fi;
  (void) args;
}

/*
 * Example of notification function:
 * function upd(ev, url) {
 *      if (ev == Sys.updater.GOT_REQUEST) {
 *         print("Starting update from", url);
 *         Sys.updater.start(url);
 *       }  else if(ev == Sys.updater.NOTHING_TODO) {
 *         print("No need to update");
 *       } else if(ev == Sys.updater.FAILED) {
 *         print("Update failed");
 *       } else if(ev == Sys.updater.COMPLETED) {
 *         print("Update completed");
 *         Sys.reboot();
 *       }
 * }
 */

void miot_updater_rpc_init(void) {
  struct mg_rpc *mg_rpc = miot_rpc_get_global();
  if (mg_rpc == NULL) return;
  mg_rpc_add_handler(mg_rpc, mg_mk_str("/v1/SWUpdate.Update"),
                     handle_update_req, NULL);
  mg_rpc_add_handler(mg_rpc, mg_mk_str("/v1/SWUpdate.Commit"),
                     handle_commit_req, NULL);
  mg_rpc_add_handler(mg_rpc, mg_mk_str("/v1/SWUpdate.Revert"),
                     handle_revert_req, NULL);
}

static void send_update_reply(struct mg_rpc_request_info *ri) {
  int status = (intptr_t) ri->user_data;
  LOG(LL_INFO, ("Sending update reply to %.*s: %d", (int) ri->src.len,
                ri->src.p, status));
  mg_rpc_send_errorf(ri, status, NULL);
  ri = NULL;
}

static void handle_mg_rpc_event(struct mg_rpc *mg_rpc, void *cb_arg,
                                enum mg_rpc_event ev, void *ev_arg) {
  if (ev != MG_RPC_EV_CHANNEL_OPEN) return;
  /*
   * We're only interested in default route.
   * TODO(rojer): We should be watching for the route to the destination of our
   * response.
   */
  const struct mg_str *dst = (const struct mg_str *) ev_arg;
  if (mg_vcmp(dst, MG_RPC_DST_DEFAULT) != 0) return;
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) cb_arg;
  send_update_reply(ri);
  mg_rpc_remove_observer(mg_rpc, handle_mg_rpc_event, ri);
}

void miot_updater_rpc_finish(int error_code, int64_t id,
                             const struct mg_str src) {
  struct mg_rpc *mg_rpc = miot_rpc_get_global();
  if (mg_rpc == NULL || id <= 0 || src.len == 0) return;
  struct mg_rpc_request_info *ri = NULL;
  ri = (struct mg_rpc_request_info *) calloc(1, sizeof(*ri));
  if (ri == NULL) goto clean;
  ri->rpc = mg_rpc;
  ri->id = id;
  ri->src = mg_strdup(src);
  if (ri->src.p == NULL) goto clean;
  ri->user_data = (void *) error_code;
  if (mg_rpc_is_connected(mg_rpc)) {
    send_update_reply(ri);
  } else {
    mg_rpc_add_observer(mg_rpc, handle_mg_rpc_event, ri);
  }
  ri = NULL;
clean:
  if (ri != NULL) mg_rpc_free_request_info(ri);
}

#endif /* MIOT_ENABLE_UPDATER_RPC && MIOT_ENABLE_RPC */
