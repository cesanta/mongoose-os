#include "fw/src/mg_updater_http.h"

#include "common/cs_dbg.h"
#include "fw/src/mg_console.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_timers.h"
#include "fw/src/mg_utils.h"

#if MG_ENABLE_UPDATER

static void fw_download_handler(struct mg_connection *c, int ev, void *p) {
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

        if (res < 0) {
          /* Error */
          CONSOLE_LOG(LL_ERROR,
                      ("Update error: %d %s", ctx->result, ctx->status_msg));
        }
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (ctx == NULL) break;

      if (is_write_finished(ctx)) updater_finalize(ctx);

      if (!is_update_finished(ctx)) {
        /* Update failed or connection was terminated by server */
        if (ctx->status_msg == NULL) ctx->status_msg = "Update failed";
        ctx->result = -1;
      } else if (is_reboot_required(ctx)) {
        CONSOLE_LOG(LL_INFO, ("Rebooting device"));
        mg_system_restart_after(100);
      }
      updater_finish(ctx);
      updater_context_free(ctx);
      c->user_data = NULL;
      break;
    }
  }
}

void mg_updater_http_start(struct update_context *ctx, const char *url) {
  CONSOLE_LOG(LL_INFO, ("Updating from: %s", url));

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

  char *extra_headers = NULL;

  struct mg_connection *c = mg_connect_http_opt(
      mg_get_mgr(), fw_download_handler, opts, url, extra_headers, NULL);

  if (c == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to connect to %s", url));
    return;
  }

  c->user_data = ctx;
}

#if MG_ENABLE_UPDATER_POST
static void handle_update_post(struct mg_connection *c, int ev, void *p) {
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
  struct update_context *ctx = (struct update_context *) c->user_data;
  if (ctx == NULL && ev != MG_EV_HTTP_MULTIPART_REQUEST) return;
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      c->user_data = updater_context_create();
      if (c->user_data == NULL) {
        mg_printf(c,
                  "HTTP/1.1 400 Bad request\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n"
                  "Failed\r\n");
        c->flags |= MG_F_SEND_AND_CLOSE;
      }
      break;
    }
    case MG_EV_HTTP_PART_BEGIN: {
      LOG(LL_DEBUG, ("MG_EV_HTTP_PART_BEGIN: %p %s %s", ctx, mp->var_name,
                     mp->file_name));
      /* We use ctx->file_name as a temp buffer for non-file variable values. */
      if (mp->file_name[0] == '\0') {
        ctx->file_name[0] = '\0';
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      LOG(LL_DEBUG, ("MG_EV_HTTP_PART_DATA: %p %s %s %d", ctx, mp->var_name,
                     mp->file_name, (int) mp->data.len));

      if (mp->file_name[0] == '\0') {
        /* It's a non-file form variable. */
        size_t l = strlen(ctx->file_name);
        size_t avail = sizeof(ctx->file_name) - l - 1;
        strncat(ctx->file_name, mp->data.p, MIN(mp->data.len, avail));
        break;
      } else if (!is_update_finished(ctx)) {
        updater_process(ctx, mp->data.p, mp->data.len);
        LOG(LL_DEBUG, ("updater_process res: %d", ctx->result));
      } else {
        /* Don't close connection just yet, not all browsers like that. */
      }
      break;
    }
    case MG_EV_HTTP_PART_END: {
      LOG(LL_DEBUG, ("MG_EV_HTTP_PART_END: %p %s %s %d", ctx, mp->var_name,
                     mp->file_name, mp->status));
      /* Part finished with an error. REQUEST_END will follow. */
      if (mp->status < 0) break;
      if (mp->file_name[0] == '\0') {
        /* It's a non-file form variable. Value is in ctx->file_name. */
        LOG(LL_DEBUG, ("Got var: %s=%s", mp->var_name, ctx->file_name));
        /* Commit timeout can be set after flashing. */
        if (strcmp(mp->var_name, "commit_timeout") == 0) {
          ctx->fctx.commit_timeout = atoi(ctx->file_name);
        }
      } else {
        /* End of the fw part, but there may still be parts with vars to follow,
         * which can modify settings (that can be applied post-flashing). */
      }
      break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST_END: {
      LOG(LL_DEBUG,
          ("MG_EV_HTTP_MULTIPART_REQUEST_END: %p %d", ctx, mp->status));
      /* Whatever happens, this is the last thing we do. */
      c->flags |= MG_F_SEND_AND_CLOSE;

      if (ctx == NULL) break;
      if (is_write_finished(ctx)) updater_finalize(ctx);
      if (!is_update_finished(ctx)) {
        ctx->result = -1;
        ctx->status_msg = "Update aborted";
        updater_finish(ctx);
      }
      if (mp->status < 0) {
        /* mp->status < 0 means connection is dead, do not send reply */
      } else {
        mg_printf(c,
                  "HTTP/1.1 %s\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n"
                  "%s\r\n",
                  ctx->result > 0 ? "200 OK" : "400 Bad request",
                  ctx->status_msg ? ctx->status_msg : "Unknown error");
        LOG(LL_ERROR, ("Update result: %d %s", ctx->result,
                       ctx->status_msg ? ctx->status_msg : "Unknown error"));
        if (is_reboot_required(ctx)) {
          LOG(LL_INFO, ("Rebooting device"));
          mg_system_restart_after(101);
        }
      }
      updater_context_free(ctx);
      c->user_data = NULL;
      break;
    }
  }
}
#endif /* MG_ENABLE_UPDATER_POST */

static struct update_context *s_ctx;
struct mg_connection *s_update_request_conn;

static void mg_update_result_cb(struct update_context *ctx) {
  if (ctx != s_ctx) return;
  if (s_update_request_conn != NULL) {
    int code = (ctx->result > 0 ? 200 : 500);
    mg_send_response_line(s_update_request_conn, code,
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n");
    mg_printf(s_update_request_conn, "%s\r\n", ctx->status_msg);
    s_update_request_conn->flags |= MG_F_SEND_AND_CLOSE;
    s_update_request_conn = NULL;
  }
  s_ctx = NULL;
}

static void mg_update_timer_cb(void *arg) {
  const char *url = (const char *) arg;
  if (s_ctx != NULL) return;
  s_ctx = updater_context_create();
  if (s_ctx == NULL) return;
  s_ctx->ignore_same_version = true;
  s_ctx->result_cb = mg_update_result_cb;
  mg_updater_http_start(s_ctx, url);
}

static void update_handler(struct mg_connection *c, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST:
    case MG_EV_HTTP_PART_BEGIN:
    case MG_EV_HTTP_PART_DATA:
    case MG_EV_HTTP_PART_END:
    case MG_EV_HTTP_MULTIPART_REQUEST_END: {
#if MG_ENABLE_UPDATER_POST
      if (get_cfg()->update.enable_post) {
        handle_update_post(c, ev, ev_data);
      } else
#endif
      {
        mg_send_response_line(c, 400,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "POST updates are disabled.");
        c->flags |= MG_F_SEND_AND_CLOSE;
      }
      return;
    }
    case MG_EV_HTTP_REQUEST: {
      if (s_ctx != NULL) {
        mg_send_response_line(c, 409,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "Another update is in progress.\r\n");
        c->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      const char *url = NULL;
      /* TODO(rojer): Take from request. */
      if (url == NULL) {
        url = get_cfg()->update.url;
      }
      if (url == NULL) {
        mg_send_response_line(c, 400,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "Update URL not specified and none is configured.\r\n");
        c->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      s_update_request_conn = c;
      mg_update_timer_cb((void *) url);
      break;
    }
    case MG_EV_CLOSE: {
      if (s_update_request_conn == c) {
        /* Client went away while waiting for response. */
        s_update_request_conn = NULL;
      }
      break;
    }
  }
  (void) ev_data;
}

static void update_action_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) return;
  struct http_message *hm = (struct http_message *) p;
  bool is_commit = (mg_vcmp(&hm->uri, "/update/commit") == 0);
  bool ok = (is_commit ? mg_upd_commit() : mg_upd_revert(false /* reboot */));
  mg_send_response_line(c, (ok ? 200 : 400),
                        "Content-Type: text/html\r\n"
                        "Connection: close");
  mg_printf(c, "\r\n%s\r\n", (ok ? "Ok" : "Error"));
  c->flags |= MG_F_SEND_AND_CLOSE;
  if (ok && !is_commit) mg_system_restart_after(100);
}

enum mg_init_result mg_updater_http_init(void) {
  struct mg_connection *lc = mg_get_http_listening_conn();
  mg_register_http_endpoint(lc, "/update/commit", update_action_handler);
  mg_register_http_endpoint(lc, "/update/revert", update_action_handler);
  mg_register_http_endpoint(lc, "/update", update_handler);
  struct sys_config_update *scu = &get_cfg()->update;
  if (scu->url != NULL && scu->interval > 0) {
    LOG(LL_INFO,
        ("Updates from %s, every %d seconds", scu->url, scu->interval));
    mg_set_c_timer(scu->interval * 1000, true /* repeat */, mg_update_timer_cb,
                   scu->url);
  }
  return MG_INIT_OK;
}

#endif /* MG_ENABLE_UPDATER */
