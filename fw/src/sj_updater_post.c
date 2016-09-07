/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifdef SJ_ENABLE_UPDATER_POST

#include "fw/src/sj_updater_post.h"

#include "fw/src/sj_sys_config.h"
#include "fw/src/sj_updater_common.h"
#include "fw/src/sj_utils.h"

void handle_update_post(struct mg_connection *c, int ev, void *p) {
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
    case MG_EV_HTTP_PART_DATA: {
      struct update_context *ctx = (struct update_context *) c->user_data;
      struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
      LOG(LL_DEBUG, ("Got %u bytes", mp->data.len))

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
      break;
    }
    case MG_EV_HTTP_PART_END: {
      struct update_context *ctx = (struct update_context *) c->user_data;
      struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;
      LOG(LL_DEBUG, ("MG_EV_HTTP_PART_END: %p %p %d", ctx, mp, mp->status));
      /* Whatever happens, this is the last thing we do. */
      c->flags |= MG_F_SEND_AND_CLOSE;

      if (ctx == NULL) break;
      if (mp->status < 0) {
        /* mp->status < 0 means connection is dead, do not send reply */
      } else {
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
            sj_system_restart_after(101);
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
    } break;
  }
}

void sj_updater_post_init(void) {
  device_register_http_endpoint("/update", handle_update_post);
}

#endif /* SJ_ENABLE_UPDATER_POST */
