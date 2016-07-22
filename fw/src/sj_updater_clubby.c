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

#if defined(SJ_ENABLE_UPDATER_CLUBBY) && !defined(DISABLE_C_CLUBBY)

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

static int fill_zip_header(char *buf, size_t buf_size,
                           const struct mg_str file_name, uint32_t file_size,
                           int *header_len) {
  if (ZIP_FILENAME_OFFSET + file_name.len > buf_size) {
    LOG(LL_ERROR, ("File name %.*s is too long", file_name.len, file_name.p));
    return -1;
  }
  LOG(LL_DEBUG, ("Fake ZIP header: name=%.*s size=%d", (int) file_name.len,
                 file_name.p, file_size));
  memset(buf, 0, buf_size);

  /* offset=0: signature */
  memcpy(buf, &c_zip_file_header_magic, 4);
  /* offset=18, compressed size */
  memcpy(buf + ZIP_COMPRESSED_SIZE_OFFSET, &file_size, 4);
  /* offset=22, uncomprressed size */
  memcpy(buf + ZIP_UNCOMPRESSED_SIZE_OFFSET, &file_size, 4);
  /* offset=26, file name length */
  memcpy(buf + ZIP_FILENAME_LEN_OFFSET, &file_name.len, 2);
  /* offset=30, file name length */
  memcpy(buf + ZIP_FILENAME_OFFSET, file_name.p, file_name.len);

  *header_len = ZIP_FILENAME_OFFSET + file_name.len;

  return 0;
}

static int do_http_connect(struct update_context *ctx, const char *url,
                           const char *extra_headers);

static void request_file(struct mg_connection *c, struct update_context *ctx,
                         const char *file_name) {
  struct mg_str host;
  unsigned int port;
  struct mg_str path;
  mg_parse_uri(mg_mk_str(ctx->base_url), NULL, NULL, &host, &port, &path, NULL,
               NULL);
  LOG(LL_DEBUG,
      ("Requestifile_sizeng file %s from path %.*s/%s and host %.*s:%d",
       file_name, path.len, path.p, file_name, host.len, host.p, port));
  mg_printf(c, "GET %.*s/%s HTTP/1.1\r\n\r\n", path.len, path.p, file_name);
  ctx->file_procesed = ctx->file_size = 0;
  strcpy(ctx->file_name, file_name);
  /* TODO (alashkin): set timeout to cancel request if no reaction */
}

static void fw_download_ev_handler(struct mg_connection *c, int ev, void *p) {
  struct mbuf *io = &c->recv_mbuf;
  struct update_context *ctx = (struct update_context *) c->user_data;
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

          if (ctx->update_type == utManifest) {
            if (ctx->file_procesed == 0) {
              char buf[100];
              int header_size = 0;
              /*
               * Since mgiot updater waits for ZIP, to keep it untouched
               * we just emulate ZIP archive by puting
               * ZIP header before file content
               */
              fill_zip_header(buf, sizeof(buf), mg_mk_str(ctx->file_name),
                              ctx->file_size, &header_size);
              updater_process(ctx, buf, header_size);
            }
          }
          mbuf_remove(io, parsed);
        }
      }

      if (io->len != 0) {
        int res = updater_process(ctx, io->buf, io->len);
        ctx->file_procesed += io->len;

        LOG(LL_DEBUG, ("Processed %d (%d) bytes, result: %d", (int) io->len,
                       ctx->file_procesed, res));

        mbuf_remove(io, io->len);

        if (ctx->update_type == utManifest &&
            ctx->file_size == ctx->file_procesed) {
          LOG(LL_DEBUG, ("%s fetched succesfully", ctx->file_name));
          sj_upd_complete_file_update(ctx->dev_ctx, ctx->file_name);

          char buf[100];
          if (sj_upd_get_next_file(ctx->dev_ctx, buf, sizeof(buf)) == 1) {
            /* There are more files to go */
            request_file(c, ctx, buf);
            return;
          } else {
            /*
             * If we are in Manifest mode and all files are fetched
             * we have to tell finalize update process explicitly
             */
            res = updater_finalize(ctx);
            LOG(LL_DEBUG, ("Finalized update"))
          }
        }

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
            updater_finish(ctx);
          }
        } else if (res < 0) {
          /* Error */
          CONSOLE_LOG(LL_ERROR,
                      ("Update error: %d %s", ctx->result, ctx->status_msg));
          notify_js(UJS_ERROR, NULL);
          sj_clubby_send_status_resp(s_clubby_reply, 1, ctx->status_msg);
        }
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ctx != NULL) {
        LOG(LL_DEBUG, ("Connection for %s is closed", ctx->file_name));

        if (ctx->update_type == utManifest &&
            ctx->file_size == ctx->file_procesed && !is_update_finished(ctx)) {
          /*
           * If type=utManifest and file is fully fetched and
           * update status ! FINISHED it means nothing, but time for next file
           */
          return;
        }

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

#ifdef MG_ENABLE_SSL
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

  struct mg_connection *c = mg_connect_http_opt(&sj_mgr, fw_download_ev_handler,
                                                opts, url, extra_headers, NULL);

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

static char *get_base_url(const struct mg_str full_url) {
  int i;
  for (i = full_url.len; i >= 0; i--) {
    if (full_url.p[i] == '/') {
      char *ret = calloc(1, i + 1);
      if (ret == NULL) {
        LOG(LL_ERROR, ("Out of memory"))
        return NULL;
      }

      strncpy(ret, full_url.p, i);
      LOG(LL_DEBUG, ("Base url=%s", ret));

      return ret;
    }
  }

  return NULL;
}

static void handle_update_req(struct clubby_event *evt, void *user_data) {
  char *blob_url = NULL;
  struct json_token section_tok = JSON_INVALID_TOKEN;
  struct json_token blob_url_tok = JSON_INVALID_TOKEN;
  struct json_token blob_type_tok = JSON_INVALID_TOKEN;

  struct json_token args = evt->request.args;

  (void) user_data;
  LOG(LL_DEBUG, ("Update request received: %.*s", evt->request.args.len,
                 evt->request.args.ptr));

  const char *reply = "Malformed request";

  if (evt->request.args.type != JSON_TYPE_OBJECT) {
    goto clean;
  }

  json_scanf(args.ptr, args.len, "{section: %T, blob_url: %T, blob_type: %T}",
             &section_tok, &blob_url_tok, &blob_type_tok);

  /*
   * TODO(alashkin): enable update for another files, not
   * firmware only
   */
  if (section_tok.len == 0 || section_tok.type != JSON_TYPE_STRING ||
      strncmp(section_tok.ptr, "firmware", section_tok.len) != 0 ||
      blob_url_tok.len == 0 || blob_url_tok.type != JSON_TYPE_STRING) {
    goto clean;
  }

  LOG(LL_DEBUG, ("blob url: %.*s blob type: %.*s", blob_url_tok.len,
                 blob_url_tok.ptr, blob_type_tok.len, blob_type_tok.ptr));

  sj_clubby_free_reply(s_clubby_reply);
  s_clubby_reply = sj_clubby_create_reply(evt);

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

  if (!notify_js(UJS_GOT_REQUEST, blob_url)) {
    enum UPDATE_TYPE ut = utZip;
    if (blob_type_tok.type == JSON_TYPE_STRING &&
        strncmp(blob_type_tok.ptr, "manifest", 8) == 0) {
      ut = utManifest;
    }
    struct update_context *ctx = updater_context_create(ut);
    if (ctx == NULL) {
      reply = "Failed to init updater";
      goto clean;
    }
    if (ut == utManifest) {
      /* TODO(alashkin): unhardcode name */
      strcpy(ctx->file_name, "manifest.json");
    }
    ctx->base_url =
        get_base_url(mg_mk_str_n(blob_url_tok.ptr, blob_url_tok.len));
    if (start_update_download(
            ctx, blob_url, ut == utZip ? NULL : "Connection: keep-alive\r\n") <
        0) {
      reply = ctx->status_msg;
      goto clean;
    }
  }

  free(blob_url);
  blob_url = NULL;

  return;

clean:
  if (blob_url != NULL) {
    free(blob_url);
  }
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

void sj_updater_clubby_init() {
  sj_clubby_register_global_command("/v1/SWUpdate.Update", handle_update_req,
                                    NULL);

  sj_clubby_register_global_command(clubby_cmd_ready, handle_clubby_ready,
                                    NULL);
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
    struct update_context *ctx = updater_context_create(utZip);
    if (ctx == NULL) {
      rcode = v7_throwf(v7, "Error", "Failed to init updater");
    } else if (start_update_download(ctx, v7_get_cstring(v7, &manifest_url_v),
                                     NULL) < 0) {
      rcode = v7_throwf(v7, "Error", ctx->status_msg);
    }
  }

  *res = v7_mk_boolean(v7, rcode == V7_OK);
  return rcode;
}

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

void sj_updater_clubby_js_init(struct v7 *v7) {
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
}
#endif /* CS_DISABLE_JS */

#endif /* defined(SJ_ENABLE_UPDATER_CLUBBY) && !defined(DISABLE_C_CLUBBY) */
