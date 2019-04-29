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

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_uart_dev_config {
/* TODO(rojer) */
#if 0
  int8_t tx_pin;
  int8_t rx_pin;
  int8_t cts_pin;
  int8_t rts_pin;
#endif
};

uint32_t cc32xx_uart_get_base(int uart_no);
bool cc32xx_uart_cts(int uart_no);
uint32_t cc32xx_uart_raw_ints(int uart_no);
uint32_t cc32xx_uart_int_mask(int uart_no);
void cc32xx_uart_early_init(int uart_no, int baud_rate);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_UART_H_ */
