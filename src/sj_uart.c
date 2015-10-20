#include <v7.h>
#include <stdlib.h>

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

  ud->cb = v7_create_undefined();
  ud->want = 0;

  s = v7_create_string(ud->v7, d, want, 1);
  sj_invoke_cb1(ud->v7, cb, s);

  return want;
}

/*
 * Usage:
 *
 * new UART("platform_specific_name")
 *
 */
static v7_val_t UART_ctor(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_arg(v7, 0);
  struct user_data *ud;
  void *uart;
  const char *name;
  size_t len;

  if (!v7_is_string(dev)) {
    v7_throw(v7, "device must be string");
  }

  ud = (struct user_data *) calloc(1, sizeof(struct user_data));
  ud->v7 = v7;
  ud->want = 0;
  ud->cb = v7_create_undefined();
  v7_own(v7, &ud->cb);

  name = v7_to_string(v7, &dev, &len);
  uart = sj_hal_open_uart(name, (void *) ud);
  if (uart == NULL) {
    v7_throw(v7, "cannot open uart");
  }

  v7_set(v7, this_obj, "_ud", ~0, V7_PROPERTY_HIDDEN, v7_create_foreign(ud));
  v7_set(v7, this_obj, "_dev", ~0, V7_PROPERTY_HIDDEN, v7_create_foreign(uart));
  return v7_create_undefined();
}

static v7_val_t UART_write(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_get(v7, this_obj, "_dev", ~0), data = v7_arg(v7, 0);
  size_t len;
  const char *d = v7_to_string(v7, &data, &len);

  (void) v7;
  (void) this_obj;

  sj_hal_write_uart(v7_to_foreign(dev), d, len);
  return v7_create_undefined();
}

/*
 * Read the content of the UART. It does not block.
 * Optional `max_len` parameter, defaults to max size_t.
 */
static v7_val_t UART_read(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t dev = v7_get(v7, this_obj, "_dev", ~0), maxv = v7_arg(v7, 0);
  size_t max = v7_is_number(maxv) ? (size_t) v7_to_number(maxv) : ~0;
  return sj_hal_read_uart(v7, v7_to_foreign(dev), max);
}

/*
 * Regiter a callback to be invoked when there is at least N bytes available.
 * N defaults to maxint if undefined
 */
static v7_val_t UART_recv(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t cb = v7_arg(v7, 0);
  v7_val_t wantv = v7_arg(v7, 1);
  v7_val_t udv = v7_get(v7, this_obj, "_ud", ~0);
  size_t want = v7_is_number(wantv) ? (size_t) v7_to_number(wantv) : ~0;

  struct user_data *ud = (struct user_data *) v7_to_foreign(udv);
  ud->cb = cb;
  v7_own(v7, &ud->cb);
  ud->want = want;
  /* TODO(mkm): trigger cb if there is already something in the buffer */
  return v7_create_undefined();
}

void sj_init_uart(struct v7 *v7) {
  v7_val_t uart = v7_create_undefined(), uart_proto = v7_create_undefined(),
           uart_ctor = v7_create_undefined();
  v7_own(v7, &uart);
  v7_own(v7, &uart_proto);
  v7_own(v7, &uart_ctor);

  uart = v7_create_object(v7);
  uart_proto = v7_create_object(v7);
  uart_ctor = v7_create_constructor(v7, uart_proto, UART_ctor, 1);

  v7_set(v7, uart, "dev", ~0, V7_PROPERTY_HIDDEN, uart_ctor);
  v7_set_method(v7, uart_proto, "read", UART_read);
  v7_set_method(v7, uart_proto, "write", UART_write);
  v7_set_method(v7, uart_proto, "recv", UART_recv);
  v7_set(v7, v7_get_global(v7), "UART", ~0, 0, uart);
  v7_exec(v7, "UART.open = function (d) { return new UART.dev(d); }", NULL);

  v7_disown(v7, &uart_ctor);
  v7_disown(v7, &uart_proto);
  v7_disown(v7, &uart);
}
