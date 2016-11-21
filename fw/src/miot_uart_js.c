/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_uart_js.h"

#include <stdlib.h>

#include "fw/src/miot_common.h"
#include "fw/src/miot_uart.h"
#include "fw/src/miot_v7_ext.h"
#include "v7/v7.h"

#if MIOT_ENABLE_JS && MG_ENABLE_UART_API

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct miot_uart_js_state {
  struct v7 *v7;
  v7_val_t obj;
  struct miot_uart_state *us;
  unsigned int recv_pending : 1;
  unsigned int tx_empty_notified : 1;
};

static struct miot_uart_js_state s_uart_js_state[MIOT_MAX_NUM_UARTS];
static v7_val_t s_uart_proto;

void miot_uart_js_dispatcher(struct miot_uart_state *us) {
  struct miot_uart_js_state *ujs =
      (struct miot_uart_js_state *) us->dispatcher_data;
  cs_rbuf_t *rxb = &us->rx_buf;
  cs_rbuf_t *txb = &us->tx_buf;
  struct v7 *v7 = ujs->v7;

  if (rxb->used > 0) {
    v7_val_t rxcb = v7_get(v7, ujs->obj, "_rxcb", ~0);
    if (v7_is_callable(v7, rxcb) && !ujs->recv_pending) {
      ujs->recv_pending = 1;
      miot_invoke_cb0_this(v7, rxcb, ujs->obj);
      /* Note: Callback has not run yet, it has been scheduled. */
    }
  }
  if (txb->used == 0) {
    v7_val_t txcb = v7_get(v7, ujs->obj, "_txcb", 5);
    if (v7_is_callable(v7, txcb) && !ujs->tx_empty_notified) {
      miot_invoke_cb0_this(v7, txcb, ujs->obj);
    }
  }
}

MG_PRIVATE enum v7_err UART_get(struct v7 *v7, v7_val_t *res) {
  enum v7_err ret = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  int uart_no = v7_get_double(v7, arg0);
  if (v7_is_number(arg0) && (uart_no == 0 || uart_no == 1)) {
    struct miot_uart_js_state *ujs = &s_uart_js_state[uart_no];
    if (ujs->v7 == NULL) {
      ujs->obj = v7_mk_object(v7);
      v7_own(v7, &ujs->obj);
      v7_set_proto(v7, ujs->obj, s_uart_proto);
      v7_set(v7, ujs->obj, "_un", ~0, v7_mk_number(v7, uart_no));
      ujs->v7 = v7;
    }
    *res = ujs->obj;
  } else {
    ret = v7_throwf(v7, "Error", "Invalid UART number");
  }
  return ret;
}

static enum v7_err get_js_state(struct v7 *v7, int *uart_no,
                                struct miot_uart_js_state **ujs,
                                bool require_us) {
  v7_val_t uart_no_v = v7_get(v7, v7_get_this(v7), "_u", ~0);
  if (v7_is_number(uart_no_v)) {
    *uart_no = v7_get_double(v7, uart_no_v);
    if (*uart_no >= 0 && *uart_no < MIOT_MAX_NUM_UARTS) {
      *ujs = &s_uart_js_state[*uart_no];
      if (require_us && (*ujs)->us == NULL) {
        return v7_throwf(v7, "Error", "Not configured");
      }
      return V7_OK;
    }
  }
  return v7_throwf(v7, "Error", "Invalid UART number");
}

#define DECLARE_UJS(require_us)                                   \
  int uart_no;                                                    \
  struct miot_uart_js_state *ujs;                                 \
  enum v7_err ret = get_js_state(v7, &uart_no, &ujs, require_us); \
  if (ret != V7_OK) return ret;

/* Args: cfg_obj */
MG_PRIVATE enum v7_err UART_configure(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(false);

  struct miot_uart_config *cfg = miot_uart_default_config();
  if (cfg == NULL) {
    return v7_throwf(v7, "Error", "Out of memory");
  }

  v7_val_t cobj = v7_arg(v7, 0);
  if (v7_is_object(cobj)) {
    v7_val_t v;
#define NUM_PROP(p)             \
  v = v7_get(v7, cobj, #p, ~0); \
  if (v7_is_number(v)) cfg->p = v7_get_double(v7, v);
#define BOOL_PROP(p)            \
  v = v7_get(v7, cobj, #p, ~0); \
  if (!v7_is_undefined(v)) cfg->p = v7_get_bool(v7, v);

    NUM_PROP(baud_rate);

    NUM_PROP(rx_buf_size);
    BOOL_PROP(rx_fc_ena);
#if CS_PLATFORM == CS_P_ESP8266
    NUM_PROP(rx_fifo_full_thresh);
    NUM_PROP(rx_fifo_fc_thresh);
    NUM_PROP(rx_fifo_alarm);
#endif
    NUM_PROP(rx_linger_micros);

    NUM_PROP(tx_buf_size);
    BOOL_PROP(tx_fc_ena);
#if CS_PLATFORM == CS_P_ESP8266
    NUM_PROP(tx_fifo_empty_thresh);
    NUM_PROP(tx_fifo_full_thresh);
    BOOL_PROP(swap_rxcts_txrts);
#endif
  }

  ujs->us = miot_uart_init(uart_no, cfg, miot_uart_js_dispatcher, ujs);
  if (ujs->us != NULL) {
    miot_uart_schedule_dispatcher(uart_no);
    *res = v7_mk_boolean(v7, 1);
  } else {
    free(cfg);
    *res = v7_mk_boolean(v7, 0);
  }
  return V7_OK;
}

/* Args: callable, disable_rx */
MG_PRIVATE enum v7_err UART_onRecv(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);
  v7_set(v7, ujs->obj, "_rxcb", ~0, v7_arg(v7, 0));
  miot_uart_set_rx_enabled(uart_no, !v7_is_truthy(v7, v7_arg(v7, 1)));
  if (v7_is_callable(v7, v7_arg(v7, 0))) {
    miot_uart_schedule_dispatcher(uart_no);
  }
  (void) res;
  return V7_OK;
}

MG_PRIVATE enum v7_err UART_recv(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);
  cs_rbuf_t *rxb = &ujs->us->rx_buf;
  size_t len = MIN((size_t) v7_get_double(v7, v7_arg(v7, 0)), rxb->used);
  uint8_t *data;
  len = cs_rbuf_get(rxb, len, &data);
  *res = v7_mk_string(v7, (const char *) data, len, 1 /* copy */);
  cs_rbuf_consume(rxb, len);
  ujs->recv_pending = 0;
  /* This is required to unblock interrupts after buffer has been filled.
   * And won't hurt in general. */
  miot_uart_schedule_dispatcher(uart_no);
  return V7_OK;
}

MG_PRIVATE enum v7_err UART_setRXEnabled(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);
  miot_uart_set_rx_enabled(uart_no, v7_is_truthy(v7, v7_arg(v7, 0)));
  miot_uart_schedule_dispatcher(uart_no);
  (void) res;
  return V7_OK;
}

MG_PRIVATE enum v7_err UART_onTXEmpty(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);
  v7_set(v7, ujs->obj, "_txcb", ~0, v7_arg(v7, 0));
  if (v7_is_callable(v7, v7_arg(v7, 0))) {
    miot_uart_schedule_dispatcher(uart_no);
  }
  (void) res;
  return V7_OK;
}

MG_PRIVATE enum v7_err UART_sendAvail(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);
  *res = v7_mk_number(v7, ujs->us->tx_buf.avail);
  return V7_OK;
}

MG_PRIVATE enum v7_err UART_send(struct v7 *v7, v7_val_t *res) {
  DECLARE_UJS(true);

  v7_val_t arg0 = v7_arg(v7, 0);
  if (!v7_is_string(arg0)) {
    return v7_throwf(v7, "Error", "String arg required");
  }
  size_t len = 0;
  const char *data = v7_get_string(v7, &arg0, &len);
  if (data != NULL && len > 0) {
    cs_rbuf_t *txb = &ujs->us->tx_buf;
    len = MIN(len, txb->avail);
    cs_rbuf_append(txb, (uint8_t *) data, len);
    miot_uart_schedule_dispatcher(uart_no);
  }
  *res = v7_mk_number(v7, len);
  return V7_OK;
}

void miot_uart_api_setup(struct v7 *v7) {
  v7_val_t uart_proto = v7_mk_object(v7);
  v7_set_method(v7, uart_proto, "configure", UART_configure);
  v7_set_method(v7, uart_proto, "onRecv", UART_onRecv);
  v7_set_method(v7, uart_proto, "recv", UART_recv);
  v7_set_method(v7, uart_proto, "setRXEnabled", UART_setRXEnabled);
  v7_set_method(v7, uart_proto, "onTXEmpty", UART_onTXEmpty);
  v7_set_method(v7, uart_proto, "sendAvail", UART_sendAvail);
  v7_set_method(v7, uart_proto, "send", UART_send);
  v7_set_method(v7, v7_get_global(v7), "UART", UART_get);
  v7_set(v7, v7_get_global(v7), "_up", ~0, uart_proto);
}

void miot_uart_js_init(struct v7 *v7) {
  s_uart_proto = v7_get(v7, v7_get_global(v7), "_up", ~0);
}

#endif /* MIOT_ENABLE_JS && MG_ENABLE_UART_API */
