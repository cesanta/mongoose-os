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

#ifndef CS_FW_PLATFORMS_STM32_NUCLEO_F746ZG_INCLUDE_STM32_SDK_HAL_H_
#define CS_FW_PLATFORMS_STM32_NUCLEO_F746ZG_INCLUDE_STM32_SDK_HAL_H_

#include "stm32f7xx_hal.h"
#include "stm32_gpio_defs.h"

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

extern RNG_HandleTypeDef hrng;
#define RNG_1 hrng

extern I2C_HandleTypeDef hi2c1;
#define I2C_DEFAULT hi2c1

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_NUCLEO_F746ZG_INCLUDE_STM32_SDK_HAL_H_ */
