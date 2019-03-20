/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_HAL_FREERTOS_INTERNAL_H_
#define CS_FW_SRC_MGOS_HAL_FREERTOS_INTERNAL_H_

#include <stdbool.h>

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif

void mgos_hal_freertos_run_mgos_task(bool start_scheduler);

extern enum mgos_init_result mgos_hal_freertos_pre_init(void);

// FreeRTOS handlers.
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_HAL_FREERTOS_INTERNAL_H_ */
