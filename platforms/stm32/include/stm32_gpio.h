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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32_sdk_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pin definition, including function.
 */
#define STM32_PIN(port, pin_num, af)                                       \
  (((((int) (af)) & 0xf) << 16) | (((((int) (port)) & 0xff) - 'A') << 4) | \
   (((int) (pin_num)) & 0xf))

/*
 * GPIO number, for mgos_gpio functions (function is always 0)
 * E.g.: mgos_gpio_write(STM32_GPIO('A', 0), 1)
 */
#define STM32_GPIO(port, pin_num) STM32_PIN((port), (pin_num), 0)

#define STM32_GPIO_PORT_A 'A'
#define STM32_GPIO_PORT_B 'B'
#define STM32_GPIO_PORT_C 'C'
#define STM32_GPIO_PORT_D 'D'
#define STM32_GPIO_PORT_E 'E'
#define STM32_GPIO_PORT_F 'F'
#define STM32_GPIO_PORT_G 'G'
#define STM32_GPIO_PORT_H 'H'
#define STM32_GPIO_PORT_I 'I'
#define STM32_GPIO_PORT_J 'J'
#define STM32_GPIO_PORT_K 'K'

#define STM32_PIN_PORT_NUM(pin_def) (((((int) (pin_def)) >> 4) & 0xff))
#define STM32_PIN_PORT(pin_def) (STM32_PIN_PORT_NUM(pin_def) + 'A')
#define STM32_PIN_NUM(pin_def) (((int) (pin_def)) & 0xf)
#define STM32_PIN_AF(pin_def) ((((int) (pin_def)) >> 16) & 0xf)
GPIO_TypeDef *stm32_gpio_port_base(int pin_def);
#define STM32_PIN_MASK(pin_def) (1 << STM32_PIN_NUM((pin_def)))

/*
 * Controls output driver characteristics of the output.
 * Refer to the datasheet for details.
 */
enum stm32_gpio_ospeed {
  STM32_GPIO_OSPEED_LOW = 0,
  STM32_GPIO_OSPEED_MEDIUM = 1,
  STM32_GPIO_OSPEED_HIGH = 2,
  STM32_GPIO_OSPEED_VERY_HIGH = 2,
};
bool stm32_gpio_set_ospeed(int pin, enum stm32_gpio_ospeed ospeed);

/*
 * Set pull up/down mode during sleep and shutdown.
 */
#ifdef STM32L4
enum mgos_gpio_pull_type;
bool stm32_gpio_set_sleep_pull(int pin, enum mgos_gpio_pull_type pull);
#endif

/* Set analog mode for the pin. adc = true - connect ro ADC input. */
bool stm32_gpio_set_mode_analog(int pin, bool adc);

#ifdef __cplusplus
}
#endif
