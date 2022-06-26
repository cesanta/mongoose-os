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

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_UART_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_UART_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_uart_dev_config {
  /*
   * A number of bytes in the hardware Rx fifo, should be between 1 and 127.
   * How full hardware Rx fifo should be before "rx fifo full" interrupt is
   * fired.
   */
  int8_t rx_fifo_full_thresh;

  /*
   * A number of bytes in the hardware Rx fifo, should be more than
   * rx_fifo_full_thresh.
   *
   * How full hardware Rx fifo should be before CTS is deasserted, telling
   * the other side to stop sending data.
   */
  int8_t rx_fifo_fc_thresh;

  /*
   * Time in uart bit intervals when "rx fifo full" interrupt fires even if
   * it's not full enough
   */
  int8_t rx_fifo_alarm;

  /*
   * A number of bytes in the hardware Tx fifo, should be between 1 and 127.
   * When the number of bytes in Tx buffer becomes less than
   * tx_fifo_empty_thresh, "tx fifo empty" interrupt fires.
   */
  int8_t tx_fifo_empty_thresh;

  /*
   * GPIO pin numbers, default values depend on UART number.
   *
   * UART 0: Rx: 3, Tx: 1, CTS: 19, RTS: 22
   * UART 1: Rx: 25, Tx: 26, CTS: 27, RTS: 13
   * UART 2: Rx: 16, Tx: 17, CTS: 14, RTS: 15
   */

  int8_t rx_gpio;
  int8_t tx_gpio;
  int8_t cts_gpio;
  int8_t rts_gpio;

  /* Use inverted polarity for Rx and Tx. */
  bool rx_inverted;
  bool tx_inverted;

  /* Half-duplex mode: if enabled, tx_en_gpio will be configured as an output
   * and will be set to tx_en_gpio_val when data is being transmitted.
   * When transmission is done, it will be returned to !tx_en_gpio_val. */
  bool hd;
  int tx_en_gpio;
  int tx_en_gpio_val;
};

int esp32_uart_rx_fifo_len(int uart_no);
int esp32_uart_tx_fifo_len(int uart_no);
bool esp32_uart_cts(int uart_no);
uint32_t esp32_uart_raw_ints(int uart_no);
uint32_t esp32_uart_int_mask(int uart_no);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_UART_H_ */
