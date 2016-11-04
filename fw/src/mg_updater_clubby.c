/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_updater_clubby.h"

#include "common/clubby/clubby.h"
#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/mg_console.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_updater_common.h"
#include "fw/src/mg_updater_http.h"
#include "fw/src/mg_utils.h"

#if MG_ENABLE_UPDATER_CLUBBY && MG_ENABLE_CLUBBY

static struct clubby_request_info *s_update_req;

static void clubby_updater_result(struct update_context *ctx) {
  if (s_update_req == NULL) return;
  if (ctx->need_reboot) {
    /* We're about to reboot, don't reply yet. */
    return;
  }
  clubby_send_errorf(s_update_req, (ctx->result > 0 ? 0 : -1), ctx->status_msg);
  s_update_req = NULL;
}

static void handle_update_req(struct clubby_request_info *ri, void *cb_arg,
                              struct clubby_frame_info *fi,
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
  strncpy(ctx->fctx.clubby_src, ri->src.p,
          MIN(ri->src.len, sizeof(ctx->fctx.clubby_src)));
  ctx->result_cb = clubby_updater_result;
  s_update_req = ri;

  mg_updater_http_start(ctx, blob_url);
  free(blob_url);
  return;

clean:
  if (blob_url != NULL) free(blob_url);
  CONSOLE_LOG(LL_ERROR, ("Failed to start update: %s", reply));
  clubby_send_errorf(ri, -1, reply);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void handle_commit_req(struct clubby_request_info *ri, void *cb_arg,
                              struct clubby_frame_info *fi,
                              struct mg_str args) {
  clubby_send_errorf(ri, mg_upd_commit() ? 0 : -1, NULL);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
  (void) args;
}

static void handle_revert_req(struct clubby_request_info *ri, void *cb_arg,
                              struct clubby_frame_info *fi,
                              struct mg_str args) {
  bool ok = mg_upd_revert(false /* reboot */);
  clubby_send_errorf(ri, ok ? 0 : -1, NULL);
  ri = NULL;
  if (ok) mg_system_restart_after(100);
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

void mg_updater_clubby_init(void) {
  struct clubby *clubby = mg_clubby_get_global();
  if (clubby == NULL) return;
  clubby_add_handler(clubby, mg_mk_str("/v1/SWUpdate.Update"),
                     handle_update_req, NULL);
  clubby_add_handler(clubby, mg_mk_str("/v1/SWUpdate.Commit"),
                     handle_commit_req, NULL);
  clubby_add_handler(clubby, mg_mk_str("/v1/SWUpdate.Revert"),
                     handle_revert_req, NULL);
}

static void send_update_reply(struct clubby_request_info *ri) {
  int status = (intptr_t) ri->user_data;
  LOG(LL_INFO, ("Sending update reply to %.*s: %d", (int) ri->src.len,
                ri->src.p, status));
  clubby_send_errorf(ri, status, NULL);
  ri = NULL;
}

static void handle_clubby_event(struct clubby *clubby, void *cb_arg,
                                enum clubby_event ev, void *ev_arg) {
  if (ev != MG_CLUBBY_EV_CHANNEL_OPEN) return;
  /*
   * We're only interested in default route.
   * TODO(rojer): We should be watching for the route to the destination of our
   * response.
   */
  const struct mg_str *dst = (const struct mg_str *) ev_arg;
  if (mg_vcmp(dst, MG_CLUBBY_DST_DEFAULT) != 0) return;
  struct clubby_request_info *ri = (struct clubby_request_info *) cb_arg;
  send_update_reply(ri);
  clubby_remove_observer(clubby, handle_clubby_event, ri);
}

void mg_updater_clubby_finish(int error_code, int64_t id,
                              const struct mg_str src) {
  struct clubby *clubby = mg_clubby_get_global();
  if (clubby == NULL || id <= 0 || src.len == 0) return;
  struct clubby_request_info *ri = NULL;
  ri = (struct clubby_request_info *) calloc(1, sizeof(*ri));
  if (ri == NULL) goto clean;
  ri->clubby = clubby;
  ri->id = id;
  ri->src = mg_strdup(src);
  if (ri->src.p == NULL) goto clean;
  ri->user_data = (void *) error_code;
  if (clubby_is_connected(clubby)) {
    send_update_reply(ri);
  } else {
    clubby_add_observer(clubby, handle_clubby_event, ri);
  }
  ri = NULL;
clean:
  if (ri != NULL) clubby_free_request_info(ri);
}

#endif /* MG_ENABLE_UPDATER_CLUBBY && MG_ENABLE_CLUBBY */
