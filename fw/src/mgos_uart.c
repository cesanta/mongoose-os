/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_uart.h"

#include "common/cs_dbg.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_utils.h"

#ifndef IRAM
#define IRAM
#endif

#ifndef MGOS_MAX_NUM_UARTS
#error Please define MGOS_MAX_NUM_UARTS
#endif

static struct mgos_uart_state *s_uart_state[MGOS_MAX_NUM_UARTS];

IRAM void mgos_uart_schedule_dispatcher(int uart_no, bool from_isr) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  mongoose_schedule_poll(from_isr);
}

void mgos_uart_dispatcher(void *arg) {
  int uart_no = (intptr_t) arg;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  mgos_lock();
  if (us->rx_enabled) mgos_uart_dev_dispatch_rx_top(us);
  mgos_uart_dev_dispatch_tx_top(us);
  if (us->dispatcher_cb != NULL) {
    us->dispatcher_cb(us);
  }
  mgos_uart_dev_dispatch_bottom(us);
  if (us->rx_buf.len == 0) mbuf_trim(&us->rx_buf);
  if (us->tx_buf.len == 0) mbuf_trim(&us->tx_buf);
  mgos_unlock();
}

size_t mgos_uart_write(int uart_no, const void *buf, size_t len) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || !us->write_enabled) return 0;
  size_t n = 0;
  while (n < len) {
    mgos_lock();
    size_t nw = MIN(len - n, mgos_uart_txb_avail(us));
    mbuf_append(&us->tx_buf, ((uint8_t *) buf) + n, nw);
    mgos_unlock();
    n += nw;
    mgos_uart_flush(uart_no);
  }
  mgos_uart_schedule_dispatcher(uart_no, false /* from_isr */);
  return len;
}

void mgos_uart_set_write_enabled(int uart_no, bool enabled) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->write_enabled = enabled;
}

void mgos_uart_flush(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  while (us->tx_buf.len > 0) {
    mgos_lock();
    mgos_uart_dev_dispatch_tx_top(us);
    mgos_unlock();
  }
  mgos_uart_dev_flush_fifo(us);
}

struct mgos_uart_state *mgos_uart_init(int uart_no,
                                       struct mgos_uart_config *cfg,
                                       mgos_uart_dispatcher_t cb,
                                       void *dispatcher_data) {
  if (uart_no < 0 || uart_no >= MGOS_MAX_NUM_UARTS) return NULL;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us != NULL) {
    mgos_uart_deinit(uart_no);
  }
  us = (struct mgos_uart_state *) calloc(1, sizeof(*us));
  us->uart_no = uart_no;
  us->cfg = cfg;
  us->write_enabled = true;
  mbuf_init(&us->rx_buf, 0);
  mbuf_init(&us->tx_buf, 0);
  us->dispatcher_cb = cb;
  us->dispatcher_data = dispatcher_data;
  if (mgos_uart_dev_init(us)) {
    s_uart_state[uart_no] = us;
  } else {
    mbuf_free(&us->rx_buf);
    mbuf_free(&us->tx_buf);
    free(us);
    return NULL;
  }
  mgos_add_poll_cb(mgos_uart_dispatcher, (void *) (intptr_t) uart_no);
  return us;
}

bool mgos_uart_is_inited(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  return (us != NULL);
}

void mgos_uart_deinit(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  s_uart_state[uart_no] = NULL;
  mgos_remove_poll_cb(mgos_uart_dispatcher, (void *) (intptr_t) uart_no);
  mgos_uart_dev_deinit(us);
  mbuf_free(&us->rx_buf);
  mbuf_free(&us->tx_buf);
  free(us);
}

void mgos_uart_set_dispatcher(int uart_no, mgos_uart_dispatcher_t cb,
                              void *dispatcher_data) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->dispatcher_cb = cb;
  us->dispatcher_data = dispatcher_data;
}

mgos_uart_dispatcher_t mgos_uart_get_dispatcher(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return NULL;
  return us->dispatcher_cb;
}

void mgos_uart_set_rx_enabled(int uart_no, bool enabled) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->rx_enabled = enabled;
  mgos_uart_dev_set_rx_enabled(us, enabled);
}

size_t mgos_uart_rxb_avail(struct mgos_uart_state *us) {
  if (us == NULL || ((int) us->rx_buf.len) > us->cfg->rx_buf_size) return 0;
  return us->cfg->rx_buf_size - us->rx_buf.len;
}

size_t mgos_uart_txb_avail(struct mgos_uart_state *us) {
  if (us == NULL || ((int) us->tx_buf.len) > us->cfg->tx_buf_size) return 0;
  return us->cfg->tx_buf_size - us->tx_buf.len;
}

struct mgos_uart_config *mgos_uart_default_config(void) {
  struct mgos_uart_config *cfg =
      (struct mgos_uart_config *) calloc(1, sizeof(*cfg));
  if (cfg == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }
  cfg->baud_rate = 115200;
  cfg->rx_buf_size = cfg->tx_buf_size = 256;
  cfg->rx_linger_micros = 15;
  mgos_uart_dev_set_defaults(cfg);
  return cfg;
}
