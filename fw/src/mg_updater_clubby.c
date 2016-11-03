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
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_updater_common.h"
#include "fw/src/mg_utils.h"
#include "fw/src/mg_v7_ext.h"

#if MG_ENABLE_UPDATER_CLUBBY && MG_ENABLE_CLUBBY

static struct clubby_request_info *s_update_req;

static int do_http_connect(struct update_context *ctx, const char *url,
                           const char *extra_headers);

static void fw_download_ev_handler(struct mg_connection *c, int ev, void *p) {
  struct mbuf *io = &c->recv_mbuf;
  struct update_context *ctx = (struct update_context *) c->user_data;
  int res = 0;
  (void) p;

  switch (ev) {
    case MG_EV_RECV: {
      if (ctx->file_size == 0) {
        LOG(LL_DEBUG, ("Looking for HTTP header"));
        struct http_message hm;
        int parsed = mg_parse_http(io->buf, io->len, &hm, 0);
        if (parsed <= 0) {
          return;
        }
        if (hm.body.len != 0) {
          LOG(LL_DEBUG, ("HTTP header: file size: %d", (int) hm.body.len));
          if (hm.body.len == (size_t) ~0) {
            CONSOLE_LOG(LL_ERROR,
                        ("Invalid content-length, perhaps chunked-encoding"));
            ctx->status_msg =
                "Invalid content-length, perhaps chunked-encoding";
            c->flags |= MG_F_CLOSE_IMMEDIATELY;
            break;
          } else {
            ctx->file_size = hm.body.len;
          }

          mbuf_remove(io, parsed);
        }
      }

      if (io->len != 0) {
        res = updater_process(ctx, io->buf, io->len);
        mbuf_remove(io, io->len);

        if (res == 0) {
          if (is_write_finished(ctx)) res = updater_finalize(ctx);
          if (res == 0) {
            /* Need more data, everything is OK */
            break;
          }
        }

        if (res > 0) {
          updater_finish(ctx);
        } else if (res < 0) {
          /* Error */
          CONSOLE_LOG(LL_ERROR,
                      ("Update error: %d %s", ctx->result, ctx->status_msg));
          if (s_update_req) {
            clubby_send_errorf(s_update_req, 1, ctx->status_msg);
            s_update_req = NULL;
          }
        }
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ctx != NULL) {
        LOG(LL_DEBUG, ("Connection for %s is closed", ctx->file_name));

        if (is_write_finished(ctx)) updater_finalize(ctx);

        if (!is_update_finished(ctx)) {
          /* Update failed or connection was terminated by server */
          if (ctx->status_msg == NULL) ctx->status_msg = "Update failed";
          if (s_update_req) {
            clubby_send_errorf(s_update_req, 1, ctx->status_msg);
            s_update_req = NULL;
          }
        } else if (is_reboot_required(ctx)) {
          /*
           * Conection is closed by updater, rebooting if required.
           */
          CONSOLE_LOG(LL_INFO, ("Rebooting device"));
          mg_system_restart_after(100);
        }
        updater_finish(ctx);
        updater_context_free(ctx);
        c->user_data = NULL;
      }

      break;
    }
  }
}

static int do_http_connect(struct update_context *ctx, const char *url,
                           const char *extra_headers) {
  LOG(LL_DEBUG, ("Connecting to: %s", url));

  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));

#if MG_ENABLE_SSL
  if (strlen(url) > 8 && strncmp(url, "https://", 8) == 0) {
    opts.ssl_server_name = get_cfg()->update.ssl_server_name;
    opts.ssl_ca_cert = get_cfg()->update.ssl_ca_file;
#ifndef cc3200
    if (opts.ssl_ca_cert == NULL) {
      /* Use global CA file if updater specific one is not set */
      opts.ssl_ca_cert = get_cfg()->tls.ca_file;
    }
#else
/*
 * SimpleLink only accepts one cert in DER format as a CA file so we can't
 * use a pre-packaged bundle and expect it to work, sadly.
 */
#endif
    opts.ssl_cert = get_cfg()->update.ssl_client_cert_file;
  }
#endif

  struct mg_connection *c = mg_connect_http_opt(
      mg_get_mgr(), fw_download_ev_handler, opts, url, extra_headers, NULL);

  if (c == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to connect to %s", url));
    return -1;
  }

  c->user_data = ctx;

  return 1;
}

static int start_update_download(struct update_context *ctx, const char *url,
                                 const char *extra_headers) {
  CONSOLE_LOG(LL_INFO, ("Updating FW"));

  if (do_http_connect(ctx, url, extra_headers) < 0) {
    ctx->status_msg = "Failed to connect update server";
    return -1;
  }

  return 1;
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
  if (start_update_download(ctx, blob_url, NULL) < 0) {
    reply = ctx->status_msg;
    goto clean;
  }

  s_update_req = ri;

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
