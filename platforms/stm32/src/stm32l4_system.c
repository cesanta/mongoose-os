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

#include "mgos_gpio.h"
#include "stm32_sdk_hal.h"
#include "stm32_system.h"

#include <stm32l4xx_ll_rcc.h>

#if STM32_SRAM2_SIZE != SRAM2_SIZE
#error Incorrect STM32_SRAM2_SIZE
#endif

uint32_t SystemCoreClock = 4000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
const uint32_t MSIRangeTable[12] = {100000,   200000,   400000,   800000,
                                    1000000,  2000000,  4000000,  8000000,
                                    16000000, 24000000, 32000000, 48000000};

void stm32_system_init(void) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |=
      ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif

  /* Reset the RCC clock configuration to the default reset state ------------*/
  RCC->CR |= RCC_CR_MSION;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON , HSION, and PLLON bits */
  RCC->CR &= 0xEAF6FFFFU;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x00001000U;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t) 0xFFFBFFFF;

  /* Disable all interrupts */
  // RCC->CIR = 0x00000000;

  __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
  __HAL_FLASH_DATA_CACHE_ENABLE();
  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

void stm32_clock_config(void) {
  RCC_ClkInitTypeDef cc;
  RCC_OscInitTypeDef oc;
  memset(&cc, 0, sizeof(cc));
  memset(&oc, 0, sizeof(oc));

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  oc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  oc.LSEState = (LSE_VALUE == 0 ? RCC_LSE_OFF : RCC_LSE_ON);
#if HSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_MSI;
  oc.MSIState = RCC_MSI_ON;
  oc.MSIClockRange = RCC_MSIRANGE_7; /* 8 MHz */
  oc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  oc.PLL.PLLM = 2;
#elif (HSE_VALUE <= 32000000) && (HSE_VALUE % 4000000 == 0)
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
  oc.HSEState = RCC_HSE_ON;
  oc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  oc.PLL.PLLM = (HSE_VALUE / 4000000);
#else
#error Only HSE between 4 and 32MHz is supported
#endif
  oc.PLL.PLLN = 40;            /* 4 * 40 = 160 MHz VCO output */
  oc.PLL.PLLP = RCC_PLLP_DIV7; /* not used */
  oc.PLL.PLLQ = RCC_PLLQ_DIV4; /* not used */
  oc.PLL.PLLR = RCC_PLLR_DIV2; /* 160 / 2 = 80 MHz PLL output */
  oc.PLL.PLLState = RCC_PLL_ON;
  HAL_RCC_OscConfig(&oc);

  cc.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  cc.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  cc.AHBCLKDivider = RCC_SYSCLK_DIV1;
  cc.APB1CLKDivider = RCC_HCLK_DIV1;
  cc.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&cc, FLASH_ACR_LATENCY_4WS);

  /* PLLSAI1 will provide 48 MHz clock for USB, RNG and SDMMC. */
  RCC_PLLSAI1InitTypeDef pllsai1_cfg = {
      .PLLSAI1Source = oc.PLL.PLLSource,
      .PLLSAI1M = oc.PLL.PLLM,
      .PLLSAI1N = 24,            /* 8 * 24 = 192 MHz VCO output */
      .PLLSAI1Q = RCC_PLLQ_DIV4, /* 192 / 4 = 48 MHz */
      .PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK,
  };
  HAL_RCCEx_EnablePLLSAI1(&pllsai1_cfg);
  __HAL_RCC_PLLCLKOUT_DISABLE(RCC_PLL_48M1CLK);
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_CLK48SEL_Msk, LL_RCC_RNG_CLKSOURCE_PLLSAI1);

#if LSE_VALUE != 0
  /* Enable MSI auto-trimming by LSE. */
  SET_BIT(RCC->CR, RCC_CR_MSIPLLEN);
#else
  CLEAR_BIT(RCC->CR, RCC_CR_MSIPLLEN);
#endif

  /* Turn off unused oscillators. */
  oc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  oc.HSIState = RCC_HSI_OFF;
#if HSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
  oc.HSEState = RCC_HSE_OFF;
#endif
#if LSE_VALUE == 0
  oc.OscillatorType |= RCC_OSCILLATORTYPE_LSE;
  oc.LSEState = RCC_LSE_OFF;
#endif
  oc.PLL.PLLState = RCC_PLL_NONE; /* Don't touch the PLL config */
  HAL_RCC_OscConfig(&oc);

  // Test frequency output.
  // HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_2);
}

IRAM void stm32_flush_caches(void) {
  __HAL_FLASH_DATA_CACHE_DISABLE();
  __HAL_FLASH_DATA_CACHE_RESET();
  __HAL_FLASH_DATA_CACHE_ENABLE();
  __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
  __HAL_FLASH_INSTRUCTION_CACHE_RESET();
  __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
}
