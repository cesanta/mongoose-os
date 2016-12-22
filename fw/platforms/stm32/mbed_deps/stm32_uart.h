/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_MBED_DEPS_STM32_UART_H_
#define CS_FW_PLATFORMS_STM32_MBED_DEPS_STM32_UART_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int miot_stm32_get_stdout_uart();
int miot_stm32_get_stderr_uart();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
