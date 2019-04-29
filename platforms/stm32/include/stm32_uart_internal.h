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

#ifndef CS_FW_PLATFORMS_STM32_INCLUDE_STM32_UART_INTERNAL_H_
#define CS_FW_PLATFORMS_STM32_INCLUDE_STM32_UART_INTERNAL_H_

#include "mgos_uart.h"

#include "stm32_sdk_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct stm32_uart_def {
  USART_TypeDef *regs;
  struct stm32_uart_pins default_pins;
};

bool stm32_uart_configure(int uart_no, const struct mgos_uart_config *cfg);
void stm32_uart_putc(int uart_no, char c);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_INCLUDE_STM32_UART_INTERNAL_H_ */
