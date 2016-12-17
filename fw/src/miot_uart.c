/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_uart.h"

#include "common/cs_dbg.h"

#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_prompt.h"
#include "fw/src/miot_utils.h"
#include "fw/src/miot_v7_ext.h"

#ifndef IRAM
#define IRAM
#endif

#ifndef MIOT_MAX_NUM_UARTS
#error Please define MIOT_MAX_NUM_UARTS
#endif

#ifndef MIOT_DEBUG_UART_BAUD_RATE
#define MIOT_DEBUG_UART_BAUD_RATE 115200
#endif

static struct miot_uart_state *s_uart_state[MIOT_MAX_NUM_UARTS];

IRAM void miot_uart_schedule_dispatcher(int uart_no) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  mongoose_schedule_poll();
}

void miot_uart_dispatcher(void *arg) {
  int uart_no = (intptr_t) arg;
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  if (us->rx_enabled) miot_uart_dev_dispatch_rx_top(us);
  miot_uart_dev_dispatch_tx_top(us);
  if (us->dispatcher_cb != NULL) {
    us->dispatcher_cb(us);
  }
  miot_uart_dev_dispatch_bottom(us);
}

size_t miot_uart_write(int uart_no, const void *buf, size_t len) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || !us->write_enabled) return 0;
  size_t n = 0;
  cs_rbuf_t *txb = &us->tx_buf;
  while (n < len) {
    size_t nw = MIN(len - n, txb->avail);
    cs_rbuf_append(txb, ((uint8_t *) buf) + n, nw);
    n += nw;
    miot_uart_flush(uart_no);
  }
  miot_uart_schedule_dispatcher(uart_no);
  return len;
}

void miot_uart_set_write_enabled(int uart_no, bool enabled) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->write_enabled = enabled;
}

void miot_uart_flush(int uart_no) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  cs_rbuf_t *txb = &us->tx_buf;
  while (txb->used > 0) {
    miot_uart_dev_dispatch_tx_top(us);
  }
}

struct miot_uart_state *miot_uart_init(int uart_no,
                                       struct miot_uart_config *cfg,
                                       miot_uart_dispatcher_t cb,
                                       void *dispatcher_data) {
  if (uart_no < 0 || uart_no >= MIOT_MAX_NUM_UARTS) return NULL;
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us != NULL) {
    miot_uart_deinit(uart_no);
  }
  us = (struct miot_uart_state *) calloc(1, sizeof(*us));
  us->uart_no = uart_no;
  us->cfg = cfg;
  us->write_enabled = true;
  cs_rbuf_init(&us->rx_buf, cfg->rx_buf_size);
  cs_rbuf_init(&us->tx_buf, cfg->tx_buf_size);
  us->dispatcher_cb = cb;
  us->dispatcher_data = dispatcher_data;
  if (miot_uart_dev_init(us)) {
    s_uart_state[uart_no] = us;
  } else {
    cs_rbuf_deinit(&us->rx_buf);
    cs_rbuf_deinit(&us->tx_buf);
    free(us);
    return NULL;
  }
  miot_add_poll_cb(miot_uart_dispatcher, (void *) (intptr_t) uart_no);
  return us;
}

bool miot_uart_is_inited(int uart_no) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  return (us != NULL);
}

void miot_uart_deinit(int uart_no) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  s_uart_state[uart_no] = NULL;
  miot_remove_poll_cb(miot_uart_dispatcher, (void *) (intptr_t) uart_no);
  miot_uart_dev_deinit(us);
  cs_rbuf_deinit(&us->rx_buf);
  cs_rbuf_deinit(&us->tx_buf);
  free(us);
}

void miot_uart_set_dispatcher(int uart_no, miot_uart_dispatcher_t cb,
                              void *dispatcher_data) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->dispatcher_cb = cb;
  us->dispatcher_data = dispatcher_data;
}

miot_uart_dispatcher_t miot_uart_get_dispatcher(int uart_no) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return NULL;
  return us->dispatcher_cb;
}

void miot_uart_set_rx_enabled(int uart_no, bool enabled) {
  struct miot_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->rx_enabled = enabled;
  miot_uart_dev_set_rx_enabled(us, enabled);
}

struct miot_uart_config *miot_uart_default_config(void) {
  struct miot_uart_config *cfg =
      (struct miot_uart_config *) calloc(1, sizeof(*cfg));
  if (cfg == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }
  cfg->baud_rate = 115200;
  cfg->rx_buf_size = cfg->tx_buf_size = 256;
  cfg->rx_linger_micros = 15;
  miot_uart_dev_set_defaults(cfg);
  return cfg;
}

enum miot_init_result miot_init_debug_uart(int uart_no) {
  if (uart_no < 0) return MIOT_INIT_OK;
  /* If already initialized, don't touch. */
  if (miot_uart_is_inited(uart_no)) return MIOT_INIT_OK;
  struct miot_uart_config *ucfg = miot_uart_default_config();
  ucfg->baud_rate = MIOT_DEBUG_UART_BAUD_RATE;
  if (miot_uart_init(uart_no, ucfg, NULL, NULL) == NULL) {
    return MIOT_INIT_UART_FAILED;
  }
  return MIOT_INIT_OK;
}
