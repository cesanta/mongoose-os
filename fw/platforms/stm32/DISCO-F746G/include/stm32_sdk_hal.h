/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_DISCO_F746G_INCLUDE_STM32_SDK_HAL_H_
#define CS_FW_PLATFORMS_STM32_DISCO_F746G_INCLUDE_STM32_SDK_HAL_H_

#include "stm32f7xx_hal.h"
#include "stm32_gpio_defs.h"

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef UART_HandleTypeDef UART_Handle;

extern UART_Handle huart1;
extern UART_Handle huart6;

#define UART_USB huart1
#define UART_2 huart6

extern RNG_HandleTypeDef hrng;
#define RNG_1 hrng

extern I2C_HandleTypeDef hi2c1;
#define I2C_DEFAULT hi2c1

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_DISCO_F746G_INCLUDE_STM32_SDK_HAL_H_ */
