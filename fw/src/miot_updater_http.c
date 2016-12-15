/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_updater_http.h"

#include "common/cs_dbg.h"
#include "fw/src/miot_console.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"
#include "fw/src/miot_utils.h"

#if MIOT_ENABLE_UPDATER

static void fw_download_handler(struct mg_connection *c, int ev, void *p) {
  struct mbuf *io = &c->recv_mbuf;
  struct update_context *ctx = (struct update_context *) c->user_data;
  int res = 0;
  struct mg_str *loc;
  (void) p;

  switch (ev) {
    case MG_EV_CONNECT: {
      int result = *((int *) p);
      if (result != 0) LOG(LL_ERROR, ("connect error: %d", result));
      break;
    }
    case MG_EV_RECV: {
      if (ctx->file_size == 0) {
        LOG(LL_DEBUG, ("Looking for HTTP header"));
        struct http_message hm;
        int parsed = mg_parse_http(io->buf, io->len, &hm, 0);
        if (parsed <= 0) {
          return;
        }
        if (hm.resp_code != 200) {
          if (hm.resp_code == 304) {
            ctx->result = 1;
            ctx->need_reboot = false;
            ctx->status_msg = "Not Modified";
            updater_finish(ctx);
          } else if ((hm.resp_code == 301 || hm.resp_code == 302) &&
                     (loc = mg_get_http_header(&hm, "Location")) != NULL) {
            /* NUL-terminate the URL. Every header must be followed by \r\n,
             * so there is deifnitely space there. */
            ((char *) loc->p)[loc->len] = '\0';
            /* We were told to look elsewhere. Detach update context from this
             * connection so that it doesn't get finalized when it's closed. */
            miot_updater_http_start(ctx, loc->p);
            c->user_data = NULL;
          } else {
            ctx->result = -hm.resp_code;
            ctx->need_reboot = false;
            ctx->status_msg = "Invalid HTTP response code";
            updater_finish(ctx);
          }
          c->flags |= MG_F_CLOSE_IMMEDIATELY;
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
        miot_system_restart_after(100);
      }
      updater_finish(ctx);
      updater_context_free(ctx);
      c->user_data = NULL;
      break;
    }
  }
}

void miot_updater_http_start(struct update_context *ctx, const char *url) {
  CONSOLE_LOG(LL_INFO, ("Update URL: %s, ct: %d, isv? %d", url,
                        ctx->fctx.commit_timeout, ctx->ignore_same_version));

  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));

#if MG_ENABLE_SSL
  if (strlen(url) > 8 && strncmp(url, "https://", 8) == 0) {
    opts.ssl_server_name = get_cfg()->update.ssl_server_name;
    opts.ssl_ca_cert = get_cfg()->update.ssl_ca_file;
    opts.ssl_cert = get_cfg()->update.ssl_client_cert_file;
  }
#endif

  char ehb[150];
  char *extra_headers = ehb;
  const struct sys_ro_vars *rv = get_ro_vars();
  mg_asprintf(&extra_headers, sizeof(ehb),
              "X-MIOT-Device-ID: %s %s\r\n"
              "X-MIOT-FW-Version: %s %s %s\r\n",
              (get_cfg()->device.id ? get_cfg()->device.id : "-"),
              rv->mac_address, rv->arch, rv->fw_version, rv->fw_id);

  struct mg_connection *c = mg_connect_http_opt(
      miot_get_mgr(), fw_download_handler, opts, url, extra_headers, NULL);

  if (extra_headers != ehb) free(extra_headers);

  if (c == NULL) {
    CONSOLE_LOG(LL_ERROR, ("Failed to connect to %s", url));
    ctx->result = -10;
    ctx->need_reboot = false;
    ctx->status_msg = "Failed to connect";
    updater_finish(ctx);
    return;
  }

  c->user_data = ctx;
  ctx->nc = c;
}

#if MIOT_ENABLE_UPDATER_POST
static void handle_update_post(struct mg_connection *c, int ev, void *p) {
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
  struct update_context *ctx = (struct update_context *) c->user_data;
  if (ctx == NULL && ev != MG_EV_HTTP_MULTIPART_REQUEST) return;
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      ctx = updater_context_create();
      if (ctx != NULL) {
        ctx->nc = c;
        c->user_data = ctx;
      } else {
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
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
        int code = (ctx->result > 0 ? 200 : 400);
        mg_send_response_line(c, code,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "%s\r\n",
                  ctx->status_msg ? ctx->status_msg : "Unknown error");
        if (is_reboot_required(ctx)) {
          LOG(LL_INFO, ("Rebooting device"));
          miot_system_restart_after(101);
        }
      }
      updater_context_free(ctx);
      c->user_data = NULL;
      break;
    }
  }
}
#endif /* MIOT_ENABLE_UPDATER_POST */

static struct update_context *s_ctx;
struct mg_connection *s_update_request_conn;

static void miot_update_result_cb(struct update_context *ctx) {
  if (ctx != s_ctx) return;
  if (s_update_request_conn != NULL) {
    int code = (ctx->result > 0 ? 200 : 500);
    mg_send_response_line(s_update_request_conn, code,
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n");
    mg_printf(s_update_request_conn, "(%d) %s\r\n", ctx->result,
              ctx->status_msg);
    s_update_request_conn->flags |= MG_F_SEND_AND_CLOSE;
    s_update_request_conn = NULL;
  }
  s_ctx = NULL;
}

static void miot_update_start(const char *url, int commit_timeout,
                              bool ignore_same_version) {
  if (s_ctx != NULL || url == NULL) return;
  s_ctx = updater_context_create();
  if (s_ctx == NULL) return;
  s_ctx->ignore_same_version = ignore_same_version;
  s_ctx->fctx.commit_timeout = commit_timeout;
  s_ctx->result_cb = miot_update_result_cb;
  miot_updater_http_start(s_ctx, url);
}

static void miot_update_timer_cb(void *arg) {
  struct sys_config_update *scu = &get_cfg()->update;
  miot_update_start(scu->url, scu->commit_timeout,
                    true /* ignore_same_version */);
  (void) arg;
}

static void update_handler(struct mg_connection *c, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST:
    case MG_EV_HTTP_PART_BEGIN:
    case MG_EV_HTTP_PART_DATA:
    case MG_EV_HTTP_PART_END:
    case MG_EV_HTTP_MULTIPART_REQUEST_END: {
#if MIOT_ENABLE_UPDATER_POST
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
      struct http_message *hm = (struct http_message *) ev_data;
      if (s_ctx != NULL) {
        mg_send_response_line(c, 409,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "Another update is in progress.\r\n");
        c->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      struct sys_config_update *scu = &get_cfg()->update;
      char *url = scu->url;
      int commit_timeout = scu->commit_timeout;
      bool ignore_same_version = true;
      struct mg_str params =
          (mg_vcmp(&hm->method, "POST") == 0 ? hm->body : hm->query_string);
      size_t buf_len = params.len;
      char *buf = calloc(params.len, 1), *p = buf;
      int len = mg_get_http_var(&params, "url", p, buf_len);
      if (len > 0) {
        url = p;
        p += len + 1;
        buf_len -= len + 1;
      }
      len = mg_get_http_var(&params, "commit_timeout", p, buf_len);
      if (len > 0) {
        commit_timeout = atoi(p);
      }
      len = mg_get_http_var(&params, "ignore_same_version", p, buf_len);
      if (len > 0) {
        ignore_same_version = (atoi(p) > 0);
      }
      if (url != NULL) {
        s_update_request_conn = c;
        miot_update_start(url, commit_timeout, ignore_same_version);
      } else {
        mg_send_response_line(c, 400,
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n");
        mg_printf(c, "Update URL not specified and none is configured.\r\n");
        c->flags |= MG_F_SEND_AND_CLOSE;
      }
      free(buf);
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
}

static void update_action_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) return;
  struct http_message *hm = (struct http_message *) p;
  bool is_commit = (mg_vcmp(&hm->uri, "/update/commit") == 0);
  bool ok =
      (is_commit ? miot_upd_commit() : miot_upd_revert(false /* reboot */));
  mg_send_response_line(c, (ok ? 200 : 400),
                        "Content-Type: text/html\r\n"
                        "Connection: close");
  mg_printf(c, "\r\n%s\r\n", (ok ? "Ok" : "Error"));
  c->flags |= MG_F_SEND_AND_CLOSE;
  if (ok && !is_commit) miot_system_restart_after(100);
}

enum miot_init_result miot_updater_http_init(void) {
  miot_register_http_endpoint("/update/commit", update_action_handler);
  miot_register_http_endpoint("/update/revert", update_action_handler);
  miot_register_http_endpoint("/update", update_handler);
  struct sys_config_update *scu = &get_cfg()->update;
  if (scu->url != NULL && scu->interval > 0) {
    LOG(LL_INFO,
        ("Updates from %s, every %d seconds", scu->url, scu->interval));
    miot_set_timer(scu->interval * 1000, true /* repeat */,
                   miot_update_timer_cb, scu->url);
  }
  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_UPDATER */
