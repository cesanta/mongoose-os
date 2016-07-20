/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_H_

#include <string.h>

#include "common/platforms/esp8266/esp_uart.h"

struct v7;

void esp_sj_uart_init();
void esp_sj_uart_set_prompt(int uart_no);

size_t esp_sj_uart_write(int uart_no, const void *buf, size_t len);

struct esp_uart_config *esp_sj_uart_default_config(int uart_no);

#ifndef CS_DISABLE_JS
void esp_sj_uart_init_js(struct v7 *v7);
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_SJ_UART_H_ */
