/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_ESP8266_ESP_UART_H_
#define CS_COMMON_PLATFORMS_ESP8266_ESP_UART_H_

#include <inttypes.h>

#include "common/cs_rbuf.h"

struct esp_uart_config {
  int uart_no;
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

  int status_interval_ms;

  /* Note: this is executed in ISR context, almost nothing can be done here. */
  void (*dispatch_cb)(int uart_no);
};

struct esp_uart_stats {
  uint32_t ints;

  uint32_t rx_ints;
  uint32_t rx_bytes;
  uint32_t rx_overflows;
  uint32_t rx_linger_conts;

  uint32_t tx_ints;
  uint32_t tx_bytes;
  uint32_t tx_throttles;
};

int esp_uart_init(struct esp_uart_config *cfg);
struct esp_uart_config *esp_uart_cfg(int uart_no);
void esp_uart_set_rx_enabled(int uart_no, int enabled);
int esp_uart_dispatch_rx_top(int uart_no);
void esp_uart_dispatch_tx_top(int uart_no);
void esp_uart_dispatch_bottom(int uart_no);
cs_rbuf_t *esp_uart_rx_buf(int uart_no);
cs_rbuf_t *esp_uart_tx_buf(int uart_no);

int rx_fifo_len(int uart_no);
int tx_fifo_len(int uart_no);

void esp_uart_flush(int uart_no);

#endif /* CS_COMMON_PLATFORMS_ESP8266_ESP_UART_H_ */
