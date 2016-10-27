/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#if MG_ENABLE_UPDATER_POST

#include "fw/src/mg_updater_post.h"

#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_updater_common.h"
#include "fw/src/mg_utils.h"

void handle_update_post(struct mg_connection *c, int ev, void *p) {
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
  struct update_context *ctx = (struct update_context *) c->user_data;
  if (ctx == NULL && ev != MG_EV_HTTP_MULTIPART_REQUEST) return;
  switch (ev) {
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      c->user_data = updater_context_create(utZip);
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
        /*
         * We can have NULL here if client sends data after completion of
         * update process
         */
        if (ctx != NULL && !is_update_finished(ctx)) {
          ctx->result = updater_process(ctx, mp->data.p, mp->data.len);
          LOG(LL_DEBUG, ("updater_process res: %d", ctx->result));
          if (ctx->result != 0) {
            updater_finish(ctx);
            /* Don't close connection just yet, not all browsers like that. */
          }
        }
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
      /* Whatever happens, this is the last thing we do. */
      c->flags |= MG_F_SEND_AND_CLOSE;

      if (ctx == NULL) break;
      if (mp->status < 0) {
        /* mp->status < 0 means connection is dead, do not send reply */
      } else {
        if (is_write_finished(ctx)) updater_finalize(ctx);
        if (is_update_finished(ctx)) {
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
        } else {
          mg_printf(c,
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "%s\n",
                    "Reached the end without finishing update");
          LOG(LL_ERROR, ("Reached the end without finishing update"));
        }
      }
      updater_context_free(ctx);
      c->user_data = NULL;
      break;
    }
  }
}

void mg_updater_post_init(void) {
  mg_register_http_endpoint(mg_get_http_listening_conn(), "/update",
                            handle_update_post);
}

#endif /* MG_ENABLE_UPDATER_POST */
