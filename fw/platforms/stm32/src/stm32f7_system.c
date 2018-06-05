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

#include "stm32_sdk_hal.h"

uint32_t SystemCoreClock = HSI_VALUE;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
#define VECT_TAB_OFFSET 0x0

void SystemInit(void) {
/* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |=
      ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
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

/* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = RAMDTCM_BASE |
              VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE |
              VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif

  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  SCB_EnableICache();
  SCB_EnableDCache();

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
}

void stm32_clock_config(void) {
  RCC_OscInitTypeDef oc;
  RCC_ClkInitTypeDef cc;
  RCC_PeriphCLKInitTypeDef pcc;

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

/* Configure the clock source and PLL. */
#if HSE_VALUE == 0
  oc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  oc.HSIState = RCC_HSI_ON;
  oc.HSICalibrationValue = 16;
  oc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  oc.PLL.PLLM = HSI_VALUE / 1000000;  // VCO input = 1 MHz
#else
  oc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
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

/* Turn off the unused oscilaltor. */
#if HSE_VALUE == 0
  oc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  oc.HSEState = RCC_HSE_OFF;
#else
  oc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  oc.HSIState = RCC_HSI_OFF;
#endif
  oc.PLL.PLLState = RCC_PLL_NONE; /* Don't touch the PLL config */
  HAL_RCC_OscConfig(&oc);
}
