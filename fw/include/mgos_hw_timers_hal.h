/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_HW_TIMERS_HAL_H_
#define CS_FW_SRC_MGOS_HW_TIMERS_HAL_H_

#include <stdbool.h>

#include "common/platform.h"

#include "mgos_init.h"
#include "mgos_timers.h"

#if CS_PLATFORM == CS_P_ESP8266
#include "esp_hw_timers.h"
#elif CS_PLATFORM == CS_P_ESP32
#include "esp32_hw_timers.h"
#elif CS_PLATFORM == CS_P_CC3200 || CS_PLATFORM == CS_P_CC3220
#include "cc32xx_hw_timers.h"
#elif CS_PLATFORM == CS_P_STM32
#include "stm32_hw_timers.h"
#else
struct mgos_hw_timer_dev_data {};
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_hw_timer_info {
  mgos_timer_id id;
  timer_callback cb;
  void *cb_arg;
  int flags;
  /* Device-specific data. */
  struct mgos_hw_timer_dev_data dev;
};

bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                            int flags);

/* Invoke this as the ISR. */
void mgos_hw_timers_isr(struct mgos_hw_timer_info *ti);

/* This will be invoked at the end of ISR, to clear the interrupt. */
void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti);

void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti);

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti);

enum mgos_init_result mgos_hw_timers_init(void);
void mgos_hw_timers_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_HW_TIMERS_HAL_H_ */
