/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/uart_register.h"

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_

int esp_uart_rx_fifo_len(int uart_no);
int esp_uart_tx_fifo_len(int uart_no);
int esp_uart_cts(int uart_no);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_UART_H_ */
