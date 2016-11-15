/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_UART_H_
#define CS_FW_SRC_MIOT_UART_H_

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "common/cs_rbuf.h"
#include "common/platform.h"

#define MIOT_MAX_NUM_UARTS 2

struct miot_uart_config {
  int baud_rate;

  int rx_buf_size;
  int rx_fc_ena;
  int rx_linger_micros;
#if CS_PLATFORM == CS_P_ESP8266
  int rx_fifo_full_thresh;
  int rx_fifo_fc_thresh;
  int rx_fifo_alarm;
#endif

  int tx_buf_size;
  int tx_fc_ena;
#if CS_PLATFORM == CS_P_ESP8266
  int tx_fifo_empty_thresh;
  int tx_fifo_full_thresh;

  int swap_rxcts_txrts;
#endif
};

struct miot_uart_stats {
  uint32_t ints;

  uint32_t rx_ints;
  uint32_t rx_bytes;
  uint32_t rx_overflows;
  uint32_t rx_linger_conts;

  uint32_t tx_ints;
  uint32_t tx_bytes;
  uint32_t tx_throttles;
};

struct miot_uart_config *miot_uart_default_config(void);

struct miot_uart_state;
typedef void (*miot_uart_dispatcher_t)(struct miot_uart_state *us);

struct miot_uart_state {
  int uart_no;
  struct miot_uart_config *cfg;
  cs_rbuf_t rx_buf;
  cs_rbuf_t tx_buf;
  unsigned int rx_enabled : 1;
  unsigned int write_enabled : 1;
  struct miot_uart_stats stats;
  miot_uart_dispatcher_t dispatcher_cb;
  void *dispatcher_data;
  void *dev_data;
};

size_t miot_uart_write(int uart_no, const void *buf, size_t len);
void miot_uart_set_write_enabled(int uart_no, bool enabled);

struct miot_uart_state *miot_uart_init(int uart_no,
                                       struct miot_uart_config *cfg,
                                       miot_uart_dispatcher_t cb,
                                       void *disptcher_data);
void miot_uart_deinit(int uart_no);

void miot_uart_set_dispatcher(int uart_no, miot_uart_dispatcher_t cb,
                              void *dispatcher_data);
miot_uart_dispatcher_t miot_uart_get_dispatcher(int uart_no);

void miot_uart_set_rx_enabled(int uart_no, bool enabled);

/* HAL */

/* Device-specific (de)initialization. */
bool miot_uart_dev_init(struct miot_uart_state *us);
void miot_uart_dev_deinit(struct miot_uart_state *us);

/* Read any available chars into rx_buf. Ints should be kept disabled. */
void miot_uart_dev_dispatch_rx_top(struct miot_uart_state *us);
/* Push chars from tx_buf out. Ints should be kept disabled. */
void miot_uart_dev_dispatch_tx_top(struct miot_uart_state *us);
/*
 * Finish this dispatch. Set up interrupts depending on the state of rx/tx bufs:
 *  - If rx_buf has availabel space, RX ints should be enabled.
 *  - if there is data to send, TX empty ints should be enabled.
 */
void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us);

void miot_uart_dev_set_rx_enabled(struct miot_uart_state *us, bool enabled);

/* Note: this is executed in ISR context, almost nothing can be done here. */
void miot_uart_schedule_dispatcher(int uart_no);

void miot_uart_flush(int uart_no);

#endif /* CS_FW_SRC_MIOT_UART_H_ */
