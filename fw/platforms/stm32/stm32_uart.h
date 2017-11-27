/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_STM32_UART_H_
#define CS_FW_PLATFORMS_STM32_STM32_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

void stm32_uart_dputc(int c);
void stm32_uart_dprintf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_STM32_UART_H_ */
