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

#include <stdlib.h>

#include "common/queue.h"

#include "mgos_gpio_hal.h"

#include <stm32_sdk_hal.h>

IRAM GPIO_TypeDef *stm32_gpio_port_base(int pin_def) {
  switch (STM32_PIN_PORT(pin_def)) {
    case 'A':
      return GPIOA;
    case 'B':
      return GPIOB;
    case 'C':
      return GPIOC;
    case 'D':
      return GPIOD;
    case 'E':
      return GPIOE;
    case 'F':
      return GPIOF;
    case 'G':
      return GPIOG;
    case 'H':
      return GPIOH;
#ifdef STM32F7
    case 'I':
      return GPIOI;
    case 'J':
      return GPIOJ;
    case 'K':
      return GPIOK;
#endif
  }
  return NULL;
}

static void stm32_gpio_port_en(int pin_def) {
  uint32_t bit = 0;
  switch (STM32_PIN_PORT(pin_def)) {
    case 'A':
      bit = RCC_AHB1ENR_GPIOAEN;
      break;
    case 'B':
      bit = RCC_AHB1ENR_GPIOBEN;
      break;
    case 'C':
      bit = RCC_AHB1ENR_GPIOCEN;
      break;
    case 'D':
      bit = RCC_AHB1ENR_GPIODEN;
      break;
    case 'E':
      bit = RCC_AHB1ENR_GPIOEEN;
      break;
    case 'F':
      bit = RCC_AHB1ENR_GPIOFEN;
      break;
    case 'G':
      bit = RCC_AHB1ENR_GPIOGEN;
      break;
    case 'H':
      bit = RCC_AHB1ENR_GPIOHEN;
      break;
#ifdef STM32F7
    case 'I':
      bit = RCC_AHB1ENR_GPIOIEN;
      break;
    case 'J':
      bit = RCC_AHB1ENR_GPIOJEN;
      break;
    case 'K':
      bit = RCC_AHB1ENR_GPIOKEN;
      break;
#endif
  }
  SET_BIT(RCC->AHB1ENR, bit);
}

const char *mgos_gpio_str(int pin_def, char buf[8]) {
  int i = 0;
  if (pin_def >= 0) {
    buf[i++] = STM32_PIN_PORT(pin_def);
    int pin_no = STM32_PIN_NUM(pin_def);
    if (pin_no < 10) {
      buf[i++] = '0' + pin_no;
    } else {
      buf[i++] = '1';
      buf[i++] = '0' + (pin_no - 10);
    }
    int pin_af = STM32_PIN_AF(pin_def);
    if (pin_af > 0) {
      buf[i++] = ':';
      if (pin_af < 10) {
        buf[i++] = '0' + pin_af;
      } else {
        buf[i++] = '1';
        buf[i++] = '0' + (pin_af - 10);
      }
    }
  } else {
    buf[i++] = '-';
  }
  buf[i++] = '\0';
  return buf;
}

IRAM bool mgos_gpio_read(int pin) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  return (regs->IDR & STM32_PIN_MASK(pin)) != 0;
}

IRAM bool mgos_gpio_read_out(int pin) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  return (regs->ODR & STM32_PIN_MASK(pin)) != 0;
}

IRAM void mgos_gpio_write(int pin, bool level) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return;
  uint32_t val = STM32_PIN_MASK(pin);
  if (!level) val <<= 16;
  regs->BSRR = val;
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  int af = STM32_PIN_AF(pin);
  GPIO_InitTypeDef gs = {
      .Pin = STM32_PIN_MASK(pin),
      .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
      .Alternate = af,
  };
  switch (mode) {
    case MGOS_GPIO_MODE_INPUT:
      gs.Mode = (af > 0 ? GPIO_MODE_AF_PP : GPIO_MODE_INPUT);
      break;
    case MGOS_GPIO_MODE_OUTPUT:
      gs.Mode = (af > 0 ? GPIO_MODE_AF_PP : GPIO_MODE_OUTPUT_PP);
      break;
    case MGOS_GPIO_MODE_OUTPUT_OD: {
      gs.Mode = (af > 0 ? GPIO_MODE_AF_OD : GPIO_MODE_OUTPUT_OD);
      break;
    }
  }
  stm32_gpio_port_en(pin);
  HAL_GPIO_Init(regs, &gs);
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  uint32_t m = 3, v = 0;
  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      break;
    case MGOS_GPIO_PULL_UP:
      v = 1;
      break;
    case MGOS_GPIO_PULL_DOWN:
      v = 2;
      break;
    default:
      return false;
  }
  m <<= STM32_PIN_NUM(pin);
  v <<= STM32_PIN_NUM(pin);
  MODIFY_REG(regs->PUPDR, m, v);
  return true;
}

void mgos_gpio_hal_int_clr(int pin) {
  /* TODO(rojer) */
}

void mgos_gpio_hal_int_done(int pin) {
  /* TODO(rojer) */
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  /* TODO(rojer) */
  return false;
}

bool mgos_gpio_enable_int(int pin) {
  /* TODO(rojer) */
  return false;
}

bool mgos_gpio_disable_int(int pin) {
  /* TODO(rojer) */
  return false;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  /* Do nothing here */
  return MGOS_INIT_OK;
}
