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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"

#include "arm_exc.h"
#include "mgos_core_dump.h"
#include "mgos_freertos.h"
#include "mgos_gpio.h"
#include "mgos_system.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  /* Override the HAL function but do nothing, FreeRTOS will take care of it. */
  (void) TickPriority;
  return 0;
}

static void stm32_set_nocache(void) {
#if STM32_NOCACHE_SIZE > 0
  extern uint8_t __nocache_start__, __nocache_end__; /* Linker symbols */

  int num_regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
  int prot_size = (&__nocache_end__ - &__nocache_start__);
  if (prot_size == 0) return;
  if (num_regions == 0) {
    LOG(LL_ERROR, ("Memory protection is requested but not available"));
    return;
  }
  if (prot_size > STM32_NOCACHE_SIZE) {
    LOG(LL_ERROR, ("Max size of protected region is %d", STM32_NOCACHE_SIZE));
    return;
  }
  /* Protected regions must be size-aligned. */
  if ((((uintptr_t) &__nocache_start__) & (STM32_NOCACHE_SIZE - 1)) != 0) {
    LOG(LL_ERROR, ("Protected region must be %d-aligned", STM32_NOCACHE_SIZE));
    return;
  }

  MPU_Region_InitTypeDef MPU_InitStruct;

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = (uintptr_t) &__nocache_start__;
// TODO: other sizes?
#if STM32_NOCACHE_SIZE != 0x400
#error STM32_NOCACHE_SIZE must be 1K
#endif
  MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

  LOG(LL_DEBUG,
      ("Marked [%p, %p) as no-cache", &__nocache_start__, &__nocache_end__));
#endif // STM32_NOCACHE_SIZE > 0
}

enum mgos_init_result mgos_freertos_pre_init() {
  stm32_set_nocache();
  return MGOS_INIT_OK;
}

uint32_t SystemCoreClockMHZ = 0;
uint32_t mgos_bitbang_n100_cal = 0;
extern void mgos_nsleep100_cal(void);

void SystemCoreClockUpdate(void) {
  uint32_t presc =
      AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos)];
  SystemCoreClock = HAL_RCC_GetSysClockFreq() >> presc;
  SystemCoreClockMHZ = SystemCoreClock / 1000000;
  mgos_nsleep100_cal();
}

#ifndef MGOS_BOOT_BUILD
static void stm32_dump_sram(void) {
  mgos_cd_write_section("SRAM", (void *) STM32_SRAM_BASE_ADDR, STM32_SRAM_SIZE);
}

#if STM32_SRAM2_SIZE > 0
static void stm32_dump_sram2(void) {
  mgos_cd_write_section("SRAM2", (void *) STM32_SRAM2_BASE_ADDR,
                        STM32_SRAM2_SIZE);
}
#endif

extern void __libc_init_array(void);

int main(void) {
  stm32_setup_int_vectors();
  if ((WWDG->CR & WWDG_CR_WDGA) != 0) {
    // WWDG was started by boot loader and is already ticking,
    // we have to reconfigure the int handler *now*.
    mgos_wdt_enable();
    mgos_wdt_set_timeout(10 /* seconds */);
  }
  stm32_system_init();
  __libc_init_array();
  stm32_clock_config();
  SystemCoreClockUpdate();

  if ((WWDG->CR & WWDG_CR_WDGA) != 0) {
    // APB clock has most likely changed, recalculate the WDT timings.
    mgos_wdt_set_timeout(10 /* seconds */);
  }

  mgos_cd_register_section_writer(arm_exc_dump_regs);
  mgos_cd_register_section_writer(stm32_dump_sram);
#if STM32_SRAM2_SIZE > 0
  mgos_cd_register_section_writer(stm32_dump_sram2);
#endif

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
  stm32_set_int_handler(SVCall_IRQn, vPortSVCHandler);
  stm32_set_int_handler(PendSV_IRQn, xPortPendSVHandler);
  stm32_set_int_handler(SysTick_IRQn, xPortSysTickHandler);
  mgos_freertos_run_mgos_task(true /* start_scheduler */);
  /* not reached */
  abort();
}
#endif  // MGOS_BOOT_BUILD

void assert_failed(uint8_t *file, uint32_t line) {
  fprintf(stderr, "assert_failed @ %s:%d\r\n", file, (int) line);
  abort();
}
