/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include <stm32_sdk_hal.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "lwip/tcpip.h"

#include "common/cs_dbg.h"

#include "mgos_app.h"
#include "mgos_hal_freertos_internal.h"
#include "mgos_init_internal.h"
#include "mgos_debug.h"
#include "mgos_debug_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_uart_internal.h"

#include "stm32_fs.h"
#include "stm32_hal.h"
#include "stm32_uart.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  /* Override the HAL function but do nothing, FreeRTOS will take care of it. */
  (void) TickPriority;
  return 0;
}

static void tcpip_init_done(void *arg) {
  *((bool *) arg) = true;
}

static void set_nocache(void) {
  extern uint8_t __nocache_start__, __nocache_end__; /* Linker symbols */

  int num_regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
  int prot_size = (&__nocache_end__ - &__nocache_start__);
  if (prot_size == 0) return;
  if (num_regions == 0) {
    LOG(LL_ERROR, ("Memory protection is requested but not available"));
    return;
  }
  if (prot_size > NOCACHE_SIZE) {
    LOG(LL_ERROR, ("Max size of protected region is %d", NOCACHE_SIZE));
    return;
  }
  /* Protected regions must be size-aligned. */
  if ((((uintptr_t) &__nocache_start__) & (NOCACHE_SIZE - 1)) != 0) {
    LOG(LL_ERROR, ("Protected region must be %d-aligned", NOCACHE_SIZE));
    return;
  }

  MPU_Region_InitTypeDef MPU_InitStruct;

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = (uintptr_t) &__nocache_start__;
#if NOCACHE_SIZE != 0x400
#error NOCACHE_SIZE must be 1K
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
}

enum mgos_init_result mgos_hal_freertos_pre_init() {
  set_nocache();
  volatile bool lwip_inited = false;
  tcpip_init(tcpip_init_done, (void *) &lwip_inited);
  while (!lwip_inited)
    ;
  return MGOS_INIT_OK;
}

void mgos_main() {
  GPIO_InitTypeDef info;
  info.Pin = LD1_Pin;
  info.Mode = GPIO_MODE_OUTPUT_PP;
  info.Pull = GPIO_NOPULL;
  info.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD1_GPIO_Port, &info);
  info.Pin = LD2_Pin;
  HAL_GPIO_Init(LD2_GPIO_Port, &info);
  info.Pin = LD3_Pin;
  HAL_GPIO_Init(LD3_GPIO_Port, &info);

  SCB_EnableICache();
  SCB_EnableDCache();

  SystemCoreClockUpdate();

  mgos_app_preinit();

  mgos_hal_freertos_run_mgos_task(true /* start_scheduler */);
  /* not reached */
  abort();
}
