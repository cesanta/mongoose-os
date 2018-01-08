/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <sys/time.h>

#include <stm32_sdk_hal.h>
#include "stm32f7xx_hal_iwdg.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "mgos_hal.h"
#include "mgos_sys_config.h"
#include "mgos_mongoose.h"
#include "mgos_timers.h"

#include "stm32_uart.h"

void mgos_dev_system_restart(void) {
  HAL_NVIC_SystemReset();
}

void device_get_mac_address(uint8_t mac[6]) {
  /* TODO(alashkin): implement */
  memset(mac, 0, 6);
}

void mgos_msleep(uint32_t msecs) {
  HAL_Delay(msecs);
}

void mgos_usleep(uint32_t usecs) {
  /* STM HAL_Tick has a milliseconds resolution */
  /* TODO(alashkin): try to use RTC timer to get usecs resolution */
  uint32_t msecs = usecs / 1000;
  if (msecs == 0) {
    msecs = 1;
  }
  mgos_msleep(msecs);
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  int i = 0;
  (void) ctx;
  do {
    uint32_t rnd;
    if (HAL_RNG_GenerateRandomNumber(&RNG_1, &rnd) != HAL_OK) {
      /* Possible if HAL is locked, fallback to timer */
      rnd = HAL_GetTick();
    }
    int copy_len = len - i;
    if (copy_len > 4) {
      copy_len = 4;
    }
    memcpy(buf + i, &rnd, copy_len);
    i += 4;
  } while (i < len);

  return 0;
}

#define IWDG_1_SECOND 128
IWDG_HandleTypeDef hiwdg = {
    .Instance = IWDG,
    .Init =
        {
         .Prescaler = IWDG_PRESCALER_256,
         .Reload = 5 * IWDG_1_SECOND,
         .Window = IWDG_WINDOW_DISABLE,
        },
};

void mgos_wdt_enable(void) {
  HAL_IWDG_Init(&hiwdg);
}

void mgos_wdt_feed(void) {
  HAL_IWDG_Refresh(&hiwdg);
}

void mgos_wdt_set_timeout(int secs) {
  uint32_t new_reload = (secs * IWDG_1_SECOND);
  if (!IS_IWDG_RELOAD(new_reload)) {
    LOG(LL_ERROR, ("Invalid WDT reload value %lu", new_reload));
    return;
  }
  hiwdg.Init.Reload = new_reload;
  HAL_IWDG_Init(&hiwdg);
}

void mgos_wdt_disable(void) {
  static bool printed = false;
  if (!printed) {
    printed = true;
    LOG(LL_ERROR, ("Once enabled, WDT cannot be disabled!"));
  }
}

void mgos_bitbang_write_bits_js(void) {
  /* TODO */
}

uint32_t mgos_get_cpu_freq(void) {
  /* TODO */
  return 0;
}

void mgos_ints_disable(void) {
  portENTER_CRITICAL();
}

void mgos_ints_enable(void) {
  portEXIT_CRITICAL();
}

SemaphoreHandle_t s_mgos_mux = NULL;

void mgos_lock_init(void) {
  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
}

void mgos_lock(void) {
  xSemaphoreTakeRecursive(s_mgos_mux, portMAX_DELAY);
}

void mgos_unlock(void) {
  xSemaphoreGiveRecursive(s_mgos_mux);
}

struct mgos_rlock_type *mgos_new_rlock(void) {
  return (struct mgos_rlock_type *) xSemaphoreCreateRecursiveMutex();
}

void mgos_rlock(struct mgos_rlock_type *l) {
  xSemaphoreTakeRecursive((SemaphoreHandle_t) l, portMAX_DELAY);
}

void mgos_runlock(struct mgos_rlock_type *l) {
  xSemaphoreGiveRecursive((SemaphoreHandle_t) l);
}
