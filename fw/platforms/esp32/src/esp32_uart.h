/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_USER_ESP_UART_H_
#define CS_FW_PLATFORMS_ESP32_USER_ESP_UART_H_

#include <stdint.h>

struct mgos_uart_dev_config {
  int8_t rx_fifo_full_thresh;
  int8_t rx_fifo_fc_thresh;
  int8_t rx_fifo_alarm;
  int8_t tx_fifo_empty_thresh;

  int8_t rx_gpio;
  int8_t tx_gpio;
  int8_t cts_gpio;
  int8_t rts_gpio;
};

int esp32_uart_rx_fifo_len(int uart_no);
int esp32_uart_tx_fifo_len(int uart_no);
bool esp32_uart_cts(int uart_no);
uint32_t esp32_uart_raw_ints(int uart_no);
uint32_t esp32_uart_int_mask(int uart_no);

#endif /* CS_FW_PLATFORMS_ESP32_USER_ESP_UART_H_ */
