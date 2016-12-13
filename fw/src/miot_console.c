/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_console.h"

#if MIOT_ENABLE_CONSOLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
#include "common/cs_frbuf.h"
#endif

#include "common/mbuf.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"

#ifndef MIOT_CONSOLE_FLUSH_INTERVAL_MS
#define MIOT_CONSOLE_FLUSH_INTERVAL_MS 50
#endif

struct console_ctx {
  struct mbuf buf;
#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
  struct cs_frbuf *fbuf;
#endif
  unsigned int initialized : 1;
  unsigned int msg_in_progress : 1;
  unsigned int request_in_flight : 1;
} s_cctx;

static int miot_console_next_msg_len(void) {
  size_t i;
  for (i = 0; i < s_cctx.buf.len; i++) {
    if (s_cctx.buf.buf[i] == '\n') return i + 1;
  }
  return 0;
}

void miot_console_putc(char c) {
  if (!s_cctx.initialized) return;
  putchar(c);
  /* If console is overfull, drop (or flush to file) old message(s). */
  size_t max_buf = get_cfg()->console.mem_buf_size;
  while (s_cctx.buf.len >= max_buf) {
    int l = miot_console_next_msg_len();
    if (l == 0) {
      l = s_cctx.buf.len;
      s_cctx.msg_in_progress = 0;
    }
#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
    if (s_cctx.fbuf != NULL) {
      cs_frbuf_append(s_cctx.fbuf, s_cctx.buf.buf, l - 1);
    }
#endif
    mbuf_remove(&s_cctx.buf, l);
  }
  /* Construct valid JSON from the get-go. */
  if (!s_cctx.msg_in_progress) {
    /* Skip empty lines */
    if (c == '\n') return;
    mbuf_append(&s_cctx.buf, "{\"msg\":\"", 8);
    s_cctx.msg_in_progress = 1;
  }
  if (c == '"' || c == '\\') {
    mbuf_append(&s_cctx.buf, "\\", 1);
  }
  if (c >= 0x20) mbuf_append(&s_cctx.buf, &c, 1);
  if (c == '\n') {
    mbuf_append(&s_cctx.buf, "\"}\n", 3);
    s_cctx.msg_in_progress = 0;
  }
}

void miot_console_printf(const char *fmt, ...) {
  char *buf = calloc(1, BUFSIZ);
  if (buf == NULL) return;
  va_list ap;
  va_start(ap, fmt);
  int len = c_vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  /* Truncate the message, don't grow the buffer. */
  if (len > 0) {
    if (len >= BUFSIZ) len = BUFSIZ - 1;
    buf[len] = '\n';
    int i;
    for (i = 0; i <= len; i++) miot_console_putc(buf[i]);
  }
  free(buf);
}

#if MIOT_ENABLE_RPC || MIOT_ENABLE_CONSOLE_FILE_BUFFER

#if MIOT_ENABLE_RPC
void mg_rpc_cb(struct mg_rpc *mg_rpc, void *cb_arg,
               struct mg_rpc_frame_info *fi, struct mg_str result,
               int error_code, struct mg_str error_msg) {
  s_cctx.request_in_flight = 0;
  (void) mg_rpc;
  (void) cb_arg;
  (void) fi;
  (void) result;
  (void) error_code;
  (void) error_msg;
}

static void miot_console_push_to_cloud(void) {
  if (!s_cctx.initialized || !get_cfg()->console.send_to_cloud) return;
  struct mg_rpc *c = miot_rpc_get_global();
  if (c == NULL || !mg_rpc_is_connected(c)) {
    /* If connection drops, do not wait for reply as it may never arrive. */
    s_cctx.request_in_flight = 0;
    return;
  }
  if (s_cctx.request_in_flight || !mg_rpc_can_send(c)) return;
#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
  /* Push backlog from the file buffer first. */
  if (s_cctx.fbuf != NULL) {
    char *msg = NULL;
    size_t len = cs_frbuf_get(s_cctx.fbuf, &msg);
    if (len > 0) {
      if (mg_rpc_callf(c, mg_mk_str("/v1/Log.Log"), mg_rpc_cb, NULL, NULL,
                       "%.*s", (int) len, msg)) {
        s_cctx.request_in_flight = 1;
      }
      return;
    }
  }
#endif
  int l = miot_console_next_msg_len();
  if (l == 0) return; /* Only send full messages. */
  if (mg_rpc_callf(c, mg_mk_str("/v1/Log.Log"), mg_rpc_cb, NULL, NULL, "%.*s",
                   (int) (l - 1), s_cctx.buf.buf)) {
    s_cctx.request_in_flight = 1;
    mbuf_remove(&s_cctx.buf, l);
    if (s_cctx.buf.len == 0) mbuf_trim(&s_cctx.buf);
  }
}

int miot_console_is_waiting_for_resp(void) {
  return s_cctx.request_in_flight;
}
#endif /* MIOT_ENABLE_RPC */

#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
static void miot_console_flush_to_file(void) {
  if (s_cctx.fbuf == NULL) return;
  int l;
  while ((l = miot_console_next_msg_len()) > 0) {
    if (!cs_frbuf_append(s_cctx.fbuf, s_cctx.buf.buf, l - 1)) return;
    mbuf_remove(&s_cctx.buf, l);
  }
}
#endif

static void miot_console_flush(void *arg) {
#if MIOT_ENABLE_RPC
  miot_console_push_to_cloud();
#endif
#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
  miot_console_flush_to_file();
#endif
  (void) arg;
}
#endif /* MIOT_ENABLE_RPC || MIOT_ENABLE_CONSOLE_FILE_BUFFER \
          */

void miot_console_init(void) {
#if MIOT_ENABLE_CONSOLE_FILE_BUFFER
  if (get_cfg()->console.log_file != NULL) {
    s_cctx.fbuf = cs_frbuf_init(get_cfg()->console.log_file,
                                get_cfg()->console.log_file_size);
    LOG(LL_INFO, ("%s %d", get_cfg()->console.log_file,
                  get_cfg()->console.log_file_size));
  }
#endif
#if MIOT_ENABLE_RPC || MIOT_ENABLE_CONSOLE_FILE_BUFFER
  miot_set_timer(MIOT_CONSOLE_FLUSH_INTERVAL_MS, 1 /* repeat */,
                 miot_console_flush, NULL /* arg */);
#endif
  s_cctx.initialized = 1;
}

#endif /* MIOT_ENABLE_CONSOLE */
