/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_UART_H_
#define CS_FW_SRC_MG_UART_H_

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "common/cs_rbuf.h"

#define MG_MAX_NUM_UARTS 2

struct mg_uart_config {
  int baud_rate;

  int rx_buf_size;
  int rx_fc_ena;
  int rx_fifo_full_thresh;
  int rx_fifo_fc_thresh;
  int rx_fifo_alarm;
  int rx_linger_micros;

  int tx_buf_size;
  int tx_fc_ena;
  int tx_fifo_empty_thresh;
  int tx_fifo_full_thresh;

  int swap_rxcts_txrts;
};

struct mg_uart_stats {
  uint32_t ints;

  uint32_t rx_ints;
  uint32_t rx_bytes;
  uint32_t rx_overflows;
  uint32_t rx_linger_conts;

  uint32_t tx_ints;
  uint32_t tx_bytes;
  uint32_t tx_throttles;
};

struct mg_uart_config *mg_uart_default_config(void);

struct mg_uart_state;
typedef void (*mg_uart_dispatcher_t)(struct mg_uart_state *us);

struct mg_uart_state {
  int uart_no;
  struct mg_uart_config *cfg;
  cs_rbuf_t rx_buf;
  cs_rbuf_t tx_buf;
  unsigned int rx_enabled : 1;
  unsigned int write_enabled : 1;
  struct mg_uart_stats stats;
  mg_uart_dispatcher_t dispatcher_cb;
  void *dispatcher_data;
  void *dev_data;
};

size_t mg_uart_write(int uart_no, const void *buf, size_t len);
void mg_uart_set_write_enabled(int uart_no, bool enabled);

struct mg_uart_state *mg_uart_init(int uart_no, struct mg_uart_config *cfg,
                                   mg_uart_dispatcher_t cb,
                                   void *disptcher_data);
void mg_uart_deinit(int uart_no);

void mg_uart_set_dispatcher(int uart_no, mg_uart_dispatcher_t cb,
                            void *dispatcher_data);
mg_uart_dispatcher_t mg_uart_get_dispatcher(int uart_no);

void mg_uart_set_rx_enabled(int uart_no, bool enabled);

/* HAL */

/* Device-specific (de)initialization. */
bool mg_uart_dev_init(struct mg_uart_state *us);
void mg_uart_dev_deinit(struct mg_uart_state *us);

/* Read any available chars into rx_buf. Ints should be kept disabled. */
void mg_uart_dev_dispatch_rx_top(struct mg_uart_state *us);
/* Push chars from tx_buf out. Ints should be kept disabled. */
void mg_uart_dev_dispatch_tx_top(struct mg_uart_state *us);
/*
 * Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 *  - If rx_buf has availabel space, RX ints should be enabled.
 *  - if there is data to send, TX empty ints should be enabled.
 */
void mg_uart_dev_dispatch_bottom(struct mg_uart_state *us);

void mg_uart_dev_set_rx_enabled(struct mg_uart_state *us, bool enabled);

/* Note: this is executed in ISR context, almost nothing can be done here. */
void mg_uart_schedule_dispatcher(int uart_no);

void mg_uart_flush(int uart_no);

#endif /* CS_FW_SRC_MG_UART_H_ */
