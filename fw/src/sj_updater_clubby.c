/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#include "fw/src/sj_updater_clubby.h"

#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "fw/src/sj_clubby.h"
#include "fw/src/sj_clubby_js.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_updater_common.h"
#include "fw/src/sj_v7_ext.h"

#ifndef CS_DISABLE_JS
#include "v7/v7.h"
#endif

#ifdef SJ_ENABLE_UPDATER_CONSOLE_LOGGING
#include "fw/src/sj_console.h"
#else
#define CONSOLE_LOG LOG
#endif

#define UPDATER_TEMP_FILE_NAME "ota_reply.dat"

enum js_update_status {
  UJS_GOT_REQUEST,
  UJS_COMPLETED,
  UJS_NOTHING_TODO,
  UJS_ERROR
};

#ifndef CS_DISABLE_JS
static struct v7 *s_v7;
static v7_val_t s_updater_notify_cb;
#endif

struct clubby_event *s_clubby_reply;
int s_clubby_upd_status;

static int notify_js(enum js_update_status us, const char *info) {
#ifndef CS_DISABLE_JS
  if (!v7_is_undefined(s_updater_notify_cb)) {
    if (info == NULL) {
      sj_invoke_cb1(s_v7, s_updater_notify_cb, v7_mk_number(s_v7, us));
    } else {
      sj_invoke_cb2(s_v7, s_updater_notify_cb, v7_mk_number(s_v7, us),
                    v7_mk_string(s_v7, info, ~0, 1));
    };

    return 1;
  }
#else
  (void) us;
  (void) info;
#endif

  return 0;
}

static void fw_download_ev_handler(struct mg_connection *c, int ev, void *p) {
  struct mbuf *io = &c->recv_mbuf;
  struct update_context *ctx = (struct update_context *) c->user_data;
  (void) p;

  switch (ev) {
    case MG_EV_RECV: {
      if (ctx->archive_size == 0) {
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
            ctx->archive_size = hm.body.len;
          }

          mbuf_remove(io, parsed);
        }
      }

      if (io->len != 0) {
        int res = updater_process(ctx, io->buf, io->len);
        LOG(LL_DEBUG, ("Processed %d bytes, result: %d", (int) io->len, res));

        mbuf_remove(io, io->len);

        if (res == 0) {
          /* Need more data, everything is OK */
          break;
        }

        if (res > 0) {
          if (!is_update_finished(ctx)) {
            /* Update terminated, but not because of error */
            notify_js(UJS_NOTHING_TODO, NULL);
            sj_clubby_send_status_resp(s_clubby_reply, 0, ctx->status_msg);
          } else {
            /* update ok */
            int len;
            char *upd_data = sj_clubby_repl_to_bytes(s_clubby_reply, &len);
            FILE *tmp_file = fopen(UPDATER_TEMP_FILE_NAME, "w");
            if (tmp_file == NULL || upd_data == NULL) {
              /* There is nothing we can do */
              free(upd_data);
              if (tmp_file) fclose(tmp_file);
              CONSOLE_LOG(LL_ERROR, ("Cannot save update status"));
            } else {
              fwrite(upd_data, 1, len, tmp_file);
              fclose(tmp_file);
              CONSOLE_LOG(LL_INFO, ("Update completed successfully"));
            }
          }
        } else if (res < 0) {
          /* Error */
          CONSOLE_LOG(LL_ERROR,
                      ("Update error: %d %s", ctx->result, ctx->status_msg));
          notify_js(UJS_ERROR, NULL);
          sj_clubby_send_status_resp(s_clubby_reply, 1, ctx->status_msg);
        }
        updater_finish(ctx);
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ctx != NULL) {
        if (!is_update_finished(ctx)) {
          /* Connection was terminated by server */
          notify_js(UJS_ERROR, NULL);
          sj_clubby_send_status_resp(s_clubby_reply, 1, "Update failed");
        } else if (is_reboot_required(ctx) && !notify_js(UJS_COMPLETED, NULL)) {
          /*
           * Conection is closed by updater, rebooting if required
           * and allowed (by JS)
           */
          CONSOLE_LOG(LL_INFO, ("Rebooting device"));
          updater_schedule_reboot(100);
        }

        if (s_clubby_reply) {
          sj_clubby_free_reply(s_clubby_reply);
          s_clubby_reply = NULL;
        }

        updater_finish(ctx);
        c->user_data = NULL;
      }

      break;
    }
  }
}

static int do_http_connect(struct update_context *ctx, const char *url) {
  LOG(LL_DEBUG, ("Connecting to: %s", url));

  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));

#ifdef MG_ENABLE_SSL
  if (strlen(url) > 8 && strncmp(url, "https://", 8) == 0) {
    opts.ssl_server_name = get_cfg()->update.ssl_server_name;
    opts.ssl_ca_cert = get_cfg()->update.ssl_ca_file;
    if (opts.ssl_ca_cert == NULL) {
      /* Use global CA file if updater specific one is not set */
      opts.ssl_ca_cert = get_cfg()->tls.ca_file;
    }
    opts.ssl_cert = get_cfg()->update.ssl_client_cert_file;
  }
#endif

  struct mg_connection *c = mg_connect_http_opt(&sj_mgr, fw_download_ev_handler,
                                                opts, url, NULL, NULL);

  if (c == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to connect to %s", url));
    return -1;
  }

  c->user_data = ctx;

  return 1;
}

static int start_update_download(struct update_context *ctx, const char *url) {
  CONSOLE_LOG(LL_INFO, ("Updating FW"));

  if (do_http_connect(ctx, url) < 0) {
    ctx->status_msg = "Failed to connect update server";
    return -1;
  }

  return 1;
}

void clubby_updater_finish(int error_code) {
  size_t len;
  char *data = cs_read_file(UPDATER_TEMP_FILE_NAME, &len);
  if (data == NULL) return; /* No file - no problem. */
  s_clubby_reply = sj_clubby_bytes_to_reply(data, len);
  if (s_clubby_reply != NULL) {
    s_clubby_upd_status = error_code;
  } else {
    LOG(LL_ERROR, ("Found invalid reply"));
  }
  remove(UPDATER_TEMP_FILE_NAME);
  free(data);
}

#ifndef CS_DISABLE_JS
static enum v7_err Updater_startupdate(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t manifest_url_v = v7_arg(v7, 0);
  if (!v7_is_string(manifest_url_v)) {
    rcode = v7_throwf(v7, "Error", "URL is not a string");
  } else {
    struct update_context *ctx = updater_context_create();
    if (ctx == NULL) {
      rcode = v7_throwf(v7, "Error", "Failed to init updater");
    } else if (start_update_download(ctx, v7_get_cstring(v7, &manifest_url_v)) <
               0) {
      rcode = v7_throwf(v7, "Error", ctx->status_msg);
    }
  }

  *res = v7_mk_boolean(v7, rcode == V7_OK);
  return rcode;
}
#endif

static void handle_clubby_ready(struct clubby_event *evt, void *user_data) {
  (void) user_data;
  if (s_clubby_reply) {
    LOG(LL_INFO, ("Sending update reply: %d", s_clubby_upd_status));
    s_clubby_reply->context = evt->context;
    sj_clubby_send_status_resp(
        s_clubby_reply, s_clubby_upd_status,
        s_clubby_upd_status == 0 ? "Updated successfully" : "Update reverted");
    sj_clubby_free_reply(s_clubby_reply);
    s_clubby_reply = NULL;
  };
}

static void handle_update_req(struct clubby_event *evt, void *user_data) {
  char *zip_url;
  struct json_token section = JSON_INVALID_TOKEN;
  struct json_token blob_url = JSON_INVALID_TOKEN;
  struct json_token args = evt->request.args;

  (void) user_data;
  LOG(LL_DEBUG, ("Update request received: %.*s", evt->request.args.len,
                 evt->request.args.ptr));

  const char *reply = "Malformed request";

  if (evt->request.args.type != JSON_TYPE_OBJECT) {
    goto bad_request;
  }

  json_scanf(args.ptr, args.len, "{section: %T, blob_url: %T}", &section,
             &blob_url);

  /*
   * TODO(alashkin): enable update for another files, not
   * firmware only
   */
  if (section.len == 0 || section.type != JSON_TYPE_STRING ||
      strncmp(section.ptr, "firmware", section.len) != 0 || blob_url.len == 0 ||
      blob_url.type != JSON_TYPE_STRING) {
    goto bad_request;
  }

  LOG(LL_DEBUG, ("zip url: %.*s", blob_url.len, blob_url.ptr));

  sj_clubby_free_reply(s_clubby_reply);
  s_clubby_reply = sj_clubby_create_reply(evt);

  /*
   * If user setup callback for updater, just call it.
   * User can start update with Sys.updater.start()
   */

  zip_url = calloc(1, blob_url.len + 1);
  if (zip_url == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Out of memory"));
    return;
  }

  memcpy(zip_url, blob_url.ptr, blob_url.len);

  if (!notify_js(UJS_GOT_REQUEST, zip_url)) {
    struct update_context *ctx = updater_context_create();
    if (ctx == NULL) {
      reply = "Failed to init updater";
    } else if (start_update_download(ctx, zip_url) < 0) {
      reply = ctx->status_msg;
    }
  }

  free(zip_url);

  return;

bad_request:
  CONSOLE_LOG(LL_ERROR, ("Failed to start update: %s", reply));
  sj_clubby_send_status_resp(evt, 1, reply);
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

#ifndef CS_DISABLE_JS
static enum v7_err Updater_notify(struct v7 *v7, v7_val_t *res) {
  v7_val_t cb = v7_arg(v7, 0);
  if (!v7_is_callable(v7, cb)) {
    printf("Invalid arguments\n");
    *res = v7_mk_boolean(v7, 0);
    return V7_OK;
  }

  s_updater_notify_cb = cb;

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}
#endif

void init_updater_clubby(struct v7 *v7) {
#ifndef CS_DISABLE_JS
  s_v7 = v7;
  v7_val_t updater = v7_mk_object(v7);
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  s_updater_notify_cb = V7_UNDEFINED;
  v7_own(v7, &s_updater_notify_cb);

  v7_def(v7, sys, "updater", ~0, V7_DESC_ENUMERABLE(0), updater);
  v7_set_method(v7, updater, "notify", Updater_notify);
  v7_set_method(v7, updater, "start", Updater_startupdate);

  v7_def(s_v7, updater, "GOT_REQUEST", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(v7, UJS_GOT_REQUEST));

  v7_def(s_v7, updater, "COMPLETED", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(v7, UJS_COMPLETED));

  v7_def(s_v7, updater, "NOTHING_TODO", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(v7, UJS_NOTHING_TODO));

  v7_def(s_v7, updater, "FAILED", ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)),
         v7_mk_number(v7, UJS_ERROR));
#else
  (void) v7;
#endif

  sj_clubby_register_global_command("/v1/SWUpdate.Update", handle_update_req,
                                    NULL);

  sj_clubby_register_global_command(clubby_cmd_ready, handle_clubby_ready,
                                    NULL);
}
