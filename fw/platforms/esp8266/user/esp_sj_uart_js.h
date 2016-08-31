/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_JS_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_JS_H_

#ifdef SJ_ENABLE_JS

#include "v7/v7.h"

void esp_sj_uart_set_prompt(int uart_no);
v7_val_t esp_sj_uart_get_recv_handler(int uart_no);

size_t esp_sj_uart_write(int uart_no, const void *buf, size_t len);

struct esp_uart_config *esp_sj_uart_default_config(int uart_no);

#endif /* SJ_ENABLE_JS */

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_JS_H_ */
