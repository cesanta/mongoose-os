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

#include "stm32_uart_internal.h"

#include "stm32_gpio.h"

struct stm32_uart_def const s_uart_defs[MGOS_MAX_NUM_UARTS] = {
    {.regs = NULL},
    {
        .regs = USART1,
        .default_pins =
            {
                .tx = STM32_PIN('A', 9, 7),
#ifdef BSP_DISCO_F746NG
                .rx = STM32_PIN('B', 7, 7),
#else
                .rx = STM32_PIN('A', 10, 7),
#endif
                .ck = STM32_PIN('A', 8, 7),
                .cts = STM32_PIN('A', 11, 7),
                .rts = STM32_PIN('A', 12, 7),
            },
    },
    {
        .regs = USART2,
        .default_pins =
            {
                .tx = STM32_PIN('A', 2, 7),
                .rx = STM32_PIN('A', 3, 7),
                .ck = STM32_PIN('A', 4, 7),
                .cts = STM32_PIN('A', 0, 7),
                .rts = STM32_PIN('A', 1, 7),
            },
    },
    {
        .regs = USART3,
        .default_pins =
            {
                .tx = STM32_PIN('D', 8, 7),
                .rx = STM32_PIN('D', 9, 7),
                .ck = STM32_PIN('D', 10, 7),
                .cts = STM32_PIN('D', 11, 7),
                .rts = STM32_PIN('D', 12, 7),
            },
    },
    {
        .regs = UART4,
        .default_pins =
            {
                .tx = STM32_PIN('A', 0, 8),
                .rx = STM32_PIN('A', 1, 8),
                .cts = STM32_PIN('B', 0, 8),
                .rts = STM32_PIN('A', 15, 8),
            },
    },
    {
        .regs = UART5,
        .default_pins =
            {
                .tx = STM32_PIN('C', 12, 8),
                .rx = STM32_PIN('D', 2, 8),
                .cts = STM32_PIN('C', 9, 7),
                .rts = STM32_PIN('C', 8, 7),
            },
    },
    {
        .regs = USART6,
        .default_pins =
            {
                .tx = STM32_PIN('G', 14, 8),
                .rx = STM32_PIN('G', 9, 8),
                .ck = STM32_PIN('G', 7, 8),
                .cts = STM32_PIN('G', 13, 8),
                .rts = STM32_PIN('G', 12, 8),
            },
    },
    {
        .regs = UART7,
        .default_pins =
            {
                .tx = STM32_PIN('F', 7, 8),
                .rx = STM32_PIN('F', 6, 8),
                .cts = STM32_PIN('F', 9, 8),
                .rts = STM32_PIN('F', 8, 8),
            },
    },
    {
        .regs = UART8,
        .default_pins =
            {
                .tx = STM32_PIN('E', 1, 8),
                .rx = STM32_PIN('E', 0, 8),
                .cts = STM32_PIN('D', 14, 8),
                .rts = STM32_PIN('D', 15, 8),
            },
    },
};
