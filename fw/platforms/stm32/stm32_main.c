/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include <stm32_sdk_hal.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

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
#include "stm32_lwip.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  /* Override the HAL function but do nothing, FreeRTOS will take care of it. */
  (void) TickPriority;
  return 0;
}

enum mgos_init_result mgos_hal_freertos_pre_init() {
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

  SystemCoreClockUpdate();

  mgos_app_preinit();

  mgos_hal_freertos_run_mgos_task(true /* start_scheduler */);
  /* not reached */
  abort();
}
