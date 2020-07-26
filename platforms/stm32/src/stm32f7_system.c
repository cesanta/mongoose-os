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

#include <string.h>

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

uint32_t SystemCoreClock = HSI_VALUE;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
#define VECT_TAB_OFFSET 0x0

#include "mgos_gpio.h"

static IRAM void stm32f7_enable_caches(void);
static IRAM void stm32f7_disable_caches(void);

void stm32_system_init(void) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |=
      ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif
  /* Set HSION bit */
  RCC->CR |= (uint32_t) 0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t) 0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t) 0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

  stm32f7_disable_caches();
  stm32f7_enable_caches();
}

void stm32_clock_config(void) {
  RCC_OscInitTypeDef oc;
  RCC_ClkInitTypeDef cc;
  RCC_PeriphCLKInitTypeDef pcc;
  memset(&cc, 0, sizeof(cc));
  memset(&oc, 0, sizeof(oc));
  memset(&pcc, 0, sizeof(pcc));

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  oc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  oc.LSEState = (LSE_VALUE == 0 ? RCC_LSE_OFF : RCC_LSE_ON);
#if HSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSI;
  oc.HSIState = RCC_HSI_ON;
  oc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  oc.PLL.PLLM = HSI_VALUE / 1000000;  // VCO input = 1 MHz
#else
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
  oc.HSEState = RCC_HSE_ON;
  oc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  oc.PLL.PLLM = HSE_VALUE / 1000000;  // VCO input = 1 MHz
#endif
  oc.PLL.PLLState = RCC_PLL_ON;
  oc.PLL.PLLN = 384;            // VCO output = 1 * 384 = 384 MHz
  oc.PLL.PLLP = RCC_PLLP_DIV2;  // SYSCLK = 384 / 2 = 192 MHz
  oc.PLL.PLLQ = 8;              // USB FS clock = 384 / 8 = 48 MHz
  HAL_RCC_OscConfig(&oc);

  cc.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  cc.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  cc.AHBCLKDivider = RCC_SYSCLK_DIV1;  // 192 MHz
  cc.APB1CLKDivider = RCC_HCLK_DIV2;   // 192 / 2 = 96 MHz
  cc.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&cc, FLASH_LATENCY_6);

  pcc.PeriphClockSelection =
      RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_USART3 |
      RCC_PERIPHCLK_UART4 | RCC_PERIPHCLK_UART5 | RCC_PERIPHCLK_USART6 |
      RCC_PERIPHCLK_UART7 | RCC_PERIPHCLK_UART8 | RCC_PERIPHCLK_CLK48;
  pcc.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  pcc.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  pcc.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  pcc.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
  pcc.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
  pcc.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
  pcc.Uart7ClockSelection = RCC_UART7CLKSOURCE_PCLK1;
  pcc.Uart8ClockSelection = RCC_UART8CLKSOURCE_PCLK1;
  pcc.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  HAL_RCCEx_PeriphCLKConfig(&pcc);

  /* Turn off unused oscillators. */
  oc.OscillatorType = 0;
#if HSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
  oc.HSEState = RCC_HSE_OFF;
#else
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSI;
  oc.HSIState = RCC_HSI_OFF;
#endif
#if LSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_LSE;
  oc.LSEState = RCC_LSE_OFF;
#endif
  oc.PLL.PLLState = RCC_PLL_NONE; /* Don't touch the PLL config */
  HAL_RCC_OscConfig(&oc);
}

static IRAM void stm32f7_disable_caches(void) {
  SCB->CCR &= ~(SCB_CCR_IC_Msk | SCB_CCR_DC_Msk);
  __DSB();
  __ISB();
  SCB->ICIALLU = 0UL; /* Flush ICache */
  FLASH->ACR &= ~(FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN);
  __DSB();
  __ISB();
}

/* D-Cache invalidation routine from ARM CM7 TRM - still not good enough! */
IRAM void inv_dcache(void) {
  __asm volatile(
      "\
      .EQU CCSIDR, 0xE000ED80 \n\
      .EQU CSSELR, 0xE000ED84 \n\
      .EQU DCISW, 0xE000EF60 \n\
      MOV r0, #0x0 \n\
      LDR r11, =CSSELR \n\
      STR r0, [r11] \n\
      DSB \n\
      LDR r11, =CCSIDR \n\
      LDR r2, [r11] \n\
      AND r1, r2, #0x7 \n\
      ADD r7, r1, #0x4 \n\
      MOVW r1, #0x3ff \n\
      ANDS r4, r1, r2, LSR #3 \n\
      MOVW r1, #0x7fff \n\
      ANDS r2, r1, r2, LSR #13 \n\
      CLZ r6, r4 \n\
      # Select Data Cache size \n\
      # Cache size identification \n\
      # Number of words in a cache line \n\
      LDR r11, =DCISW \n\
inv_loop1: \n\
      MOV r1, r4 \n\
inv_loop2: \n\
      LSL r3, r1, r6 \n\
      LSL r8, r2, r7 \n\
      ORR r3, r3, r8 \n\
      STR r3, [r11] \n\
      # Invalidate D-cache line \n\
      SUBS r1, r1, #0x1 \n\
      BGE inv_loop2 \n\
      SUBS r2, r2, #0x1 \n\
      BGE inv_loop1 \n\
      DSB \n\
      ISB \n\
"
      :
      :
      : "r0", "r1", "r2", "r3", "r4", "r6", "r7", "r8", "r11");
}

static IRAM void stm32f7_enable_caches(void) {
  /*
   * When re-enabling data cache here weird crashes happen when flash is erased.
   * For now, we just keep the cache disabled.
   * TODO(rojer): Figure it out.
   */
  // inv_dcache();
  // SCB->CCR |= (SCB_CCR_IC_Msk | SCB_CCR_DC_Msk);
  SCB->CCR |= SCB_CCR_IC_Msk;
  FLASH->ACR |= FLASH_ACR_ARTRST;
  __DSB();
  __ISB();
  FLASH->ACR &= ~(FLASH_ACR_ARTRST);
  __DSB();
  __ISB();
  FLASH->ACR |= (FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN);
}

IRAM void stm32_flush_caches(void) {
  stm32f7_disable_caches();
  stm32f7_enable_caches();
}
