/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_

#include <inttypes.h>

#include "common/platforms/esp8266/uart_register.h"

int esp_uart_rx_fifo_len(int uart_no);
int esp_uart_tx_fifo_len(int uart_no);
bool esp_uart_cts(int uart_no);
uint32_t esp_uart_raw_ints(int uart_no);
uint32_t esp_uart_int_mask(int uart_no);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_ */
