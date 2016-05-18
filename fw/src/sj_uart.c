/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/v7.h"
#include <stdlib.h>
#include <assert.h>

#include "sj_uart.h"
#include "sj_v7_ext.h"

struct user_data {
  struct v7 *v7;
  v7_val_t cb;
  size_t want;
};

size_t sj_uart_recv_cb(void *ctx, const char *d, size_t len) {
  struct user_data *ud = (struct user_data *) ctx;
  v7_val_t s, cb = ud->cb;
  size_t want = ud->want;

  if (len < want || v7_is_undefined(ud->cb)) return 0;

  ud->cb = V7_UNDEFINED;
  ud->want = 0;

  s = v7_mk_string(ud->v7, d, want, 1);
  sj_invoke_cb1(ud->v7, cb, s);

  return want;
}

/*
 * Usage:
 *
 * new UART("platform_specific_name")
 *
 */
static enum v7_err UART_ctor(struct v7 *v7, v7_val_t *res) {
  (void) res;
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_arg(v7, 0);
  struct user_data *ud;
  void *uart;
  const char *name;
  size_t len;

  if (!v7_is_string(dev)) {
    rcode = v7_throwf(v7, "Error", "device must be string");
    goto clean;
  }

  ud = (struct user_data *) calloc(1, sizeof(struct user_data));
  ud->v7 = v7;
  ud->want = 0;
  ud->cb = V7_UNDEFINED;
  v7_own(v7, &ud->cb);

  name = v7_get_string(v7, &dev, &len);
  uart = sj_hal_open_uart(name, (void *) ud);
  if (uart == NULL) {
    rcode = v7_throwf(v7, "Error", "cannot open uart");
    goto clean;
  }

  v7_def(v7, this_obj, "_ud", ~0, _V7_DESC_HIDDEN(1), v7_mk_foreign(v7, ud));
  v7_def(v7, this_obj, "_dev", ~0, _V7_DESC_HIDDEN(1), v7_mk_foreign(v7, uart));

clean:
  return rcode;
}

static enum v7_err UART_write(struct v7 *v7, v7_val_t *res) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_get(v7, this_obj, "_dev", ~0), data = v7_arg(v7, 0);
  size_t len;
  const char *d = v7_get_string(v7, &data, &len);

  (void) v7;
  (void) this_obj;

  sj_hal_write_uart(v7_get_ptr(v7, dev), d, len);
  *res = v7_mk_boolean(v7, 1);

  return V7_OK;
}

/*
 * Read the content of the UART. It does not block.
 * Optional `max_len` parameter, defaults to max size_t.
 */
static enum v7_err UART_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_get(v7, this_obj, "_dev", ~0), maxv = v7_arg(v7, 0);
  size_t max =
      v7_is_number(maxv) ? (size_t) v7_get_double(v7, maxv) : (size_t) ~0;
  *res = sj_hal_read_uart(v7, v7_get_ptr(v7, dev), max);

  return V7_OK;
}

/*
 * Regiter a callback to be invoked when there is at least N bytes available.
 * N defaults to maxint if undefined
 */
static enum v7_err UART_recv(struct v7 *v7, v7_val_t *res) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t cb = v7_arg(v7, 0);
  v7_val_t wantv = v7_arg(v7, 1);
  v7_val_t udv = v7_get(v7, this_obj, "_ud", ~0);
  size_t want =
      v7_is_number(wantv) ? (size_t) v7_get_double(v7, wantv) : (size_t) ~0;

  struct user_data *ud = (struct user_data *) v7_get_ptr(v7, udv);
  ud->cb = cb;
  v7_own(v7, &ud->cb);
  ud->want = want;
  /* TODO(mkm): trigger cb if there is already something in the buffer */

  *res = v7_mk_boolean(v7, 1);

  return V7_OK;
}

void sj_init_uart(struct v7 *v7) {
  v7_val_t uart = V7_UNDEFINED, uart_proto = V7_UNDEFINED,
           uart_ctor = V7_UNDEFINED;
  v7_own(v7, &uart);
  v7_own(v7, &uart_proto);
  v7_own(v7, &uart_ctor);

  uart = v7_mk_object(v7);
  uart_proto = v7_mk_object(v7);
  uart_ctor = v7_mk_function_with_proto(v7, UART_ctor, uart_proto);

  v7_def(v7, uart, "dev", ~0, _V7_DESC_HIDDEN(1), uart_ctor);
  v7_set_method(v7, uart_proto, "read", UART_read);
  v7_set_method(v7, uart_proto, "write", UART_write);
  v7_set_method(v7, uart_proto, "recv", UART_recv);
  v7_set(v7, v7_get_global(v7), "UART", ~0, uart);
  {
    enum v7_err rcode = v7_exec(
        v7, "UART.open = function (d) { return new UART.dev(d); }", NULL);
    assert(rcode == V7_OK);
#if defined(NDEBUG)
    (void) rcode;
#endif
  }

  v7_disown(v7, &uart_ctor);
  v7_disown(v7, &uart_proto);
  v7_disown(v7, &uart);
}
