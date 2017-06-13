/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_

#include <stdint.h>

struct mgos_uart_dev_config {
  int8_t rx_fifo_full_thresh;
  int8_t rx_fifo_fc_thresh;
  int8_t rx_fifo_alarm;
  int8_t tx_fifo_empty_thresh;

  bool swap_rxcts_txrts;
};

#include "common/platforms/esp8266/uart_register.h"

void esp_uart_tx_byte(int uart_no, uint8_t byte);
int esp_uart_rx_fifo_len(int uart_no);
int esp_uart_tx_fifo_len(int uart_no);
bool esp_uart_cts(int uart_no);
uint32_t esp_uart_raw_ints(int uart_no);
uint32_t esp_uart_int_mask(int uart_no);

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_ */
