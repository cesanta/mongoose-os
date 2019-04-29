/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_uart_dev_config {
  int8_t rx_fifo_full_thresh;
  int8_t rx_fifo_fc_thresh;
  int8_t rx_fifo_alarm;
  int8_t tx_fifo_empty_thresh;

  bool swap_rxcts_txrts;
};

#include "esp_uart_register.h"

void esp_uart_tx_byte(int uart_no, uint8_t byte);
int esp_uart_rx_fifo_len(int uart_no);
int esp_uart_tx_fifo_len(int uart_no);
bool esp_uart_cts(int uart_no);
uint32_t esp_uart_raw_ints(int uart_no);
uint32_t esp_uart_int_mask(int uart_no);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_UART_H_ */
