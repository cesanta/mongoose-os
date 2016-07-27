#include "fw/platforms/cc3200/src/cc3200_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>

#include "fw/platforms/cc3200/src/config.h"

#ifdef SJ_ENABLE_JS
#include "v7/v7.h"
#endif

#ifdef SJ_ENABLE_CLUBBY
#include "common/mbuf.h"
#include "fw/src/sj_clubby.h"
#include "fw/src/sj_clubby_js.h"
#include "fw/src/sj_common.h"
#include "fw/src/sj_sys_config.h"

struct console_ctx {
  struct mbuf buf;
  unsigned int initialized : 1;
  unsigned int msg_in_progress : 1;
  unsigned int request_in_flight : 1;
  unsigned int in_console : 1;
} s_cctx;

int cc3200_console_next_msg_len() {
  for (int i = 0; i < s_cctx.buf.len; i++) {
    if (s_cctx.buf.buf[i] == '\n') return i + 1;
  }
  return 0;
}

void cc3200_console_cloud_putc(char c) {
  if (s_cctx.in_console || !s_cctx.initialized) return;
  s_cctx.in_console = 1;
  /* If console is overfull, drop old message(s). */
  int max_buf = get_cfg()->console.mem_cache_size;
  while (s_cctx.buf.len >= max_buf) {
    int l = cc3200_console_next_msg_len();
    if (l == 0) {
      l = s_cctx.buf.len;
      s_cctx.msg_in_progress = 0;
    }
    mbuf_remove(&s_cctx.buf, l);
  }
  /* Construct valid JSON from the get-go. */
  if (!s_cctx.msg_in_progress) {
    /* Skip empty lines */
    if (c == '\n') goto out;
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
out:
  s_cctx.in_console = 0;
}

void clubby_cb(struct clubby_event *evt, void *user_data) {
  (void) evt;
  (void) user_data;
  s_cctx.request_in_flight = 0;
}

void cc3200_console_cloud_push() {
  if (s_cctx.buf.len == 0 || !s_cctx.initialized) return;
  int l = cc3200_console_next_msg_len();
  if (l == 0) return;  // Only send full messages.
  struct clubby *c = sj_clubby_get_global();
  if (c == NULL || !sj_clubby_is_connected(c)) {
    /* If connection drops, do not wait for reply as it may never arrive. */
    s_cctx.request_in_flight = 0;
    return;
  }
  if (s_cctx.request_in_flight || !sj_clubby_can_send(c)) return;
  s_cctx.request_in_flight = 1;
  s_cctx.in_console = 1;
  sj_clubby_call(c, NULL, "/v1/Log.Log", mg_mk_str_n(s_cctx.buf.buf, l - 1), 0,
                 clubby_cb, NULL);
  mbuf_remove(&s_cctx.buf, l);
  if (s_cctx.buf.len == 0) mbuf_trim(&s_cctx.buf);
  s_cctx.in_console = 0;
}

void sj_console_cloud_log(const char *fmt, ...) {
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
    for (int i = 0; i <= len; i++) cc3200_console_cloud_putc(buf[i]);
  }
  free(buf);
}

int sj_console_is_waiting_for_resp() {
  return s_cctx.request_in_flight;
}
#endif /* SJ_ENABLE_CLUBBY */

#ifdef SJ_ENABLE_JS
static void puts_n(const char *s, int len) {
  while (len-- > 0) {
    putchar(*s);
    s++;
  }
}

SJ_PRIVATE enum v7_err Console_log(struct v7 *v7, v7_val_t *res) {
  int argc = v7_argc(v7);
  /* Put everything into one message */
  for (int i = 0; i < argc; i++) {
    v7_val_t arg = v7_arg(v7, i);
    if (v7_is_string(arg)) {
      size_t len;
      const char *str = v7_get_string(v7, &arg, &len);
      puts_n(str, len);
    } else {
      char buf[100], *p;
      p = v7_stringify(v7, arg, buf, sizeof(buf), V7_STRINGIFY_DEBUG);
      puts_n(p, strlen(p));
      if (p != buf) free(p);
    }
    if (i != argc - 1) {
      putchar(' ');
    }
  }
  putchar('\n');

  *res = V7_UNDEFINED; /* like JS print */
  return V7_OK;
}

void sj_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_own(v7, &console_v);

  v7_set_method(v7, console_v, "log", Console_log);
  v7_set(v7, v7_get_global(v7), "console", ~0, console_v);

  v7_disown(v7, &console_v);
}

void sj_console_js_init(struct v7 *v7) {
}
#endif /* SJ_ENABLE_JS */

void sj_console_init() {
#ifdef SJ_ENABLE_CLUBBY
  s_cctx.initialized = 1;
#endif
}

void cc3200_console_putc(int fd, char c) {
  MAP_UARTCharPut(CONSOLE_UART, c);
#ifdef SJ_ENABLE_CLUBBY
  if (fd == 1 && get_cfg()->console.send_to_cloud) cc3200_console_cloud_putc(c);
#endif
  (void) fd;
}
