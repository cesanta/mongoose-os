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

#include "stm32_gpio.h"

#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_gpio_hal.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

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
#ifdef STM32F7
    case 'H': /* GPIOH is not supported on the F4 and L4 */
      return GPIOH;
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

#if defined(STM32F2) || defined(STM32F4) || defined(STM32F7)
static bool stm32_gpio_port_en(int pin_def) {
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
#ifdef STM32F7
    case 'H':
      bit = RCC_AHB1ENR_GPIOHEN;
      break;
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
    default:
      return false;
  }
  SET_BIT(RCC->AHB1ENR, bit);
  return true;
}
#elif defined(STM32L4)
static bool stm32_gpio_port_en(int pin_def) {
  uint32_t bit = 0;
  switch (STM32_PIN_PORT(pin_def)) {
    case 'A':
      bit = RCC_AHB2ENR_GPIOAEN;
      break;
    case 'B':
      bit = RCC_AHB2ENR_GPIOBEN;
      break;
    case 'C':
      bit = RCC_AHB2ENR_GPIOCEN;
      break;
    case 'D':
      bit = RCC_AHB2ENR_GPIODEN;
      break;
    case 'E':
      bit = RCC_AHB2ENR_GPIOEEN;
      break;
    case 'F':
      bit = RCC_AHB2ENR_GPIOFEN;
      break;
    case 'G':
      bit = RCC_AHB2ENR_GPIOGEN;
      break;
#if 0 /* We don't support GPIOH yet */
    case 'H':
      bit = RCC_AHB2ENR_GPIOHEN;
      break;
#endif
    default:
      return false;
  }
  SET_BIT(RCC->AHB2ENR, bit);
  return true;
}

/* L4 has two sets of EXTI registers but GPIO stuff is in set 1. */
#define PR PR1
#define IMR IMR1
#define RTSR RTSR1
#define FTSR FTSR1

#endif

const char *mgos_gpio_str(int pin_def, char buf[8]) {
  int i = 0;
  if (pin_def >= 0) {
    buf[i++] = 'P';
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
      buf[i++] = '.';
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
  const uint32_t af = STM32_PIN_AF(pin);
  const uint32_t pin_num = STM32_PIN_NUM(pin);
  uint32_t afr_msk = (0xf << ((pin_num & 7) * 4));
  uint32_t afr_val = (af << ((pin_num & 7) * 4));
  volatile uint32_t *afrp = &regs->AFR[pin_num < 8 ? 0 : 1];
  uint32_t moder_msk = (3 << (pin_num * 2)), moder_val = 0;
  uint32_t otyper_msk = (1 << pin_num), otyper_val = 0;
  switch (mode) {
    case MGOS_GPIO_MODE_INPUT:
      break;
    case MGOS_GPIO_MODE_OUTPUT:
      moder_val = 1;
      break;
    case MGOS_GPIO_MODE_OUTPUT_OD:
      moder_val = 1;
      otyper_val = 1;
      break;
  }
  if (!stm32_gpio_port_en(pin)) return false;
  MODIFY_REG(*afrp, afr_msk, afr_val);
  if (afr_val != 0) moder_val = 2;
  MODIFY_REG(regs->MODER, moder_msk, (moder_val << (pin_num * 2)));
  if (moder_val == 1 || moder_val == 2) {
    MODIFY_REG(regs->OTYPER, otyper_msk, (otyper_val << pin_num));
    stm32_gpio_set_ospeed(pin, STM32_GPIO_OSPEED_VERY_HIGH);
  }
  return true;
}

bool stm32_gpio_set_mode_analog(int pin, bool adc) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  if (!stm32_gpio_port_en(pin)) return false;
  const uint32_t pin_num = STM32_PIN_NUM(pin);
  uint32_t moder_msk = (3 << (pin_num * 2));
  MODIFY_REG(regs->MODER, moder_msk, moder_msk);  // Pin mode 3 (analog).
#ifdef GPIO_ASCR_ASC0
  MODIFY_REG(regs->ASCR, (1 << pin_num), (((uint32_t) adc) << pin_num));
#else
  (void) adc;
#endif
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  const uint32_t shift = STM32_PIN_NUM(pin) * 2;
  uint32_t pupdr_val = 0;
  switch (pull) {
    case MGOS_GPIO_PULL_NONE:
      break;
    case MGOS_GPIO_PULL_UP:
      pupdr_val = 1;
      break;
    case MGOS_GPIO_PULL_DOWN:
      pupdr_val = 2;
      break;
    default:
      return false;
  }
  MODIFY_REG(regs->PUPDR, (3 << shift), (pupdr_val << shift));
#ifdef STM32L4
  // Start with sleep mode pull disabled, user will enable if required.
  stm32_gpio_set_sleep_pull(pin, MGOS_GPIO_PULL_NONE);
#endif
  return true;
}

#ifdef STM32L4
bool stm32_gpio_set_sleep_pull(int pin, enum mgos_gpio_pull_type pull) {
  if (pull != MGOS_GPIO_PULL_NONE) {
    SET_BIT(PWR->CR3, PWR_CR3_APC);
  }
  int port_no = (STM32_PIN_PORT(pin) - 'A');
  uint32_t pin_mask = (1 << STM32_PIN_NUM(pin));
  volatile uint32_t *pu_reg = &PWR->PUCRA + port_no * 2;
  if (pull == MGOS_GPIO_PULL_UP) {
    SET_BIT(*pu_reg, pin_mask);
  } else {
    CLEAR_BIT(*pu_reg, pin_mask);
  }
  volatile uint32_t *pd_reg = &PWR->PDCRA + port_no * 2;
  if (pull == MGOS_GPIO_PULL_DOWN) {
    SET_BIT(*pd_reg, pin_mask);
  } else {
    CLEAR_BIT(*pd_reg, pin_mask);
  }
  if (pull != MGOS_GPIO_PULL_NONE) {
    HAL_PWREx_EnablePullUpPullDownConfig();
  }
  return true;
}
#endif

bool mgos_gpio_setup_output(int pin, bool level) {
  if (!stm32_gpio_port_en(pin)) return false;
  mgos_gpio_write(pin, level);
  return mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
}

bool stm32_gpio_set_ospeed(int pin, enum stm32_gpio_ospeed ospeed) {
  GPIO_TypeDef *regs = stm32_gpio_port_base(pin);
  if (regs == NULL) return false;
  const uint32_t pin_num = STM32_PIN_NUM(pin);
  uint32_t ospeedr_msk = (3 << (pin_num * 2));
  uint32_t ospeedr = ((((uint32_t) ospeed) & 3) << (pin_num * 2));
  MODIFY_REG(regs->OSPEEDR, ospeedr_msk, ospeedr);
  return true;
}

/*
 * STM32 supports up to 16 GPIO int sources simultaneously (EXTIn),
 * mapped to 7 IRQ lines in the NVIC>
 * Pxy ports are mapped to EXTIn such that y always maps to n
 * but which Px is used is selected by SYSCFG_EXTICR.
 *
 * I.e. Px0 will trigger EXTI0, Px1 -> EXTI1, ..., Px15 -> EXTI15.
 * Selection of x is performed via EXTICR.
 *
 * This means that it is not possible to have interrupts on both PA0 and PB0.
 */

/* Find out which port this EXTI line is assigned to. */
static int exti_selected_port_num(int exti_num) {
  uint32_t exticr_num = (exti_num / 4);
  uint32_t exticr_shl = (exti_num % 4) * 4;
  uint32_t port_num = ((SYSCFG->EXTICR[exticr_num] >> exticr_shl) & 0xf);
  return port_num;
}

static void stm32_gpio_ext_int_handler(uint32_t exti_min, uint32_t exti_max) {
  uint32_t exti_msk = (1 << exti_min);
  for (uint32_t i = exti_min; i <= exti_max; i++, exti_msk <<= 1) {
    if (!(EXTI->PR & exti_msk)) continue;
    mgos_gpio_hal_int_cb(STM32_GPIO(exti_selected_port_num(i) + 'A', i));
  }
}

static void stm32_gpio_exti0_int_handler(void) {
  stm32_gpio_ext_int_handler(0, 0);
}
static void stm32_gpio_exti1_int_handler(void) {
  stm32_gpio_ext_int_handler(1, 1);
}
static void stm32_gpio_exti2_int_handler(void) {
  stm32_gpio_ext_int_handler(2, 2);
}
static void stm32_gpio_exti3_int_handler(void) {
  stm32_gpio_ext_int_handler(3, 3);
}
static void stm32_gpio_exti4_int_handler(void) {
  stm32_gpio_ext_int_handler(4, 4);
}
static void stm32_gpio_exti_5_9_int_handler(void) {
  stm32_gpio_ext_int_handler(5, 9);
}
static void stm32_gpio_exti_10_15_int_handler(void) {
  stm32_gpio_ext_int_handler(10, 15);
}

void mgos_gpio_hal_clear_int(int pin) {
  uint32_t exti_num = STM32_PIN_NUM(pin);
  uint32_t exti_msk = (1 << exti_num);
  EXTI->PR = exti_msk;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  uint32_t port_num = STM32_PIN_PORT_NUM(pin);
  uint32_t exti_num = STM32_PIN_NUM(pin);
  uint32_t exti_msk = (1 << exti_num);
  uint32_t rtr_val = 0, ftr_val = 0;
  switch (mode) {
    case MGOS_GPIO_INT_NONE:
      break;
    case MGOS_GPIO_INT_EDGE_POS:
      rtr_val = exti_msk;
      break;
    case MGOS_GPIO_INT_EDGE_NEG:
      ftr_val = exti_msk;
      break;
    case MGOS_GPIO_INT_EDGE_ANY:
      rtr_val = ftr_val = exti_msk;
      break;
    case MGOS_GPIO_INT_LEVEL_HI:
    case MGOS_GPIO_INT_LEVEL_LO:
    default:
      return false;
  }
  if ((EXTI->RTSR & exti_msk) || (EXTI->FTSR & exti_msk)) {
    uint32_t cur_port_num = exti_selected_port_num(exti_num);
    if (cur_port_num != port_num) {
      char buf1[8], buf2[8];
      LOG(LL_ERROR,
          ("EXTI%d is already assigned to %s (want: %s)", (int) exti_num,
           mgos_gpio_str(STM32_GPIO(cur_port_num + 'A', exti_num), buf1),
           mgos_gpio_str(pin, buf2)));
      (void) buf1;
      (void) buf2;
      return false;
    }
  }
  CLEAR_BIT(EXTI->IMR, exti_msk);
  MODIFY_REG(EXTI->RTSR, exti_msk, rtr_val);
  MODIFY_REG(EXTI->FTSR, exti_msk, ftr_val);
  EXTI->PR = exti_msk;
  uint32_t exticr_num = (exti_num / 4);
  uint32_t exticr_shl = (exti_num % 4) * 4;
  uint32_t exticr_msk = (0xf << exticr_shl);
  uint32_t exticr_val = (port_num << exticr_shl);
  MODIFY_REG(SYSCFG->EXTICR[exticr_num], exticr_msk, exticr_val);
  return true;
}

bool mgos_gpio_hal_enable_int(int pin) {
  uint32_t exti_num = STM32_PIN_NUM(pin);
  uint32_t exti_msk = (1 << exti_num);
  if (exti_selected_port_num(exti_num) != STM32_PIN_PORT_NUM(pin)) {
    return false;
  }
  SET_BIT(EXTI->IMR, exti_msk);
  return true;
}

bool mgos_gpio_hal_disable_int(int pin) {
  uint32_t exti_num = STM32_PIN_NUM(pin);
  uint32_t exti_msk = (1 << exti_num);
  if (exti_selected_port_num(exti_num) != STM32_PIN_PORT_NUM(pin)) {
    return false;
  }
  CLEAR_BIT(EXTI->IMR, exti_msk);
  return true;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  const struct {
    int irqn;
    void (*handler)(void);
  } ext_irqs[7] = {
      {EXTI0_IRQn, stm32_gpio_exti0_int_handler},
      {EXTI1_IRQn, stm32_gpio_exti1_int_handler},
      {EXTI2_IRQn, stm32_gpio_exti2_int_handler},
      {EXTI3_IRQn, stm32_gpio_exti3_int_handler},
      {EXTI4_IRQn, stm32_gpio_exti4_int_handler},
      {EXTI9_5_IRQn, stm32_gpio_exti_5_9_int_handler},
      {EXTI15_10_IRQn, stm32_gpio_exti_10_15_int_handler},
  };
  EXTI->PR = 0xffff;
  MODIFY_REG(EXTI->IMR, 0xffff, 0);
  for (int i = 0; i < (int) ARRAY_SIZE(ext_irqs); i++) {
    int irqn = ext_irqs[i].irqn;
    stm32_set_int_handler(irqn, ext_irqs[i].handler);
    HAL_NVIC_SetPriority(irqn, 9, 0);
    HAL_NVIC_EnableIRQ(irqn);
  }
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  SYSCFG->EXTICR[0] = 0;
  SYSCFG->EXTICR[1] = 0;
  SYSCFG->EXTICR[2] = 0;
  SYSCFG->EXTICR[3] = 0;
  return MGOS_INIT_OK;
}
