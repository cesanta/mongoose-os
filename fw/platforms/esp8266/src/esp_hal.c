/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/esp_missing_includes.h"

#ifdef RTOS_SDK
#include "esp_misc.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#endif

#include "common/cs_dbg.h"

#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"

#include "fw/src/mgos_debug.h"
#include "fw/src/mgos_mongoose.h"
#include "common/umm_malloc/umm_malloc.h"

#include "fw/platforms/esp8266/src/esp_fs.h"
#include "fw/platforms/esp8266/src/esp_hw_wdt.h"

size_t mgos_get_heap_size(void) {
  return UMM_MALLOC_CFG__HEAP_SIZE;
}

size_t mgos_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t mgos_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

void mgos_wdt_disable(void) {
  esp_hw_wdt_disable();
}

void mgos_wdt_enable(void) {
  esp_hw_wdt_enable();
}

void mgos_wdt_feed(void) {
  esp_hw_wdt_feed();
}

void mgos_wdt_set_timeout(int secs) {
  esp_hw_wdt_setup(esp_hw_wdt_secs_to_timeout(secs), ESP_HW_WDT_1_68_SEC);
}

void mgos_system_restart(int exit_code) {
  (void) exit_code;
  fs_umount();
  LOG(LL_INFO, ("Restarting"));
  mgos_debug_flush();
  system_restart();
}

void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

void mgos_usleep(uint32_t usecs) {
#ifdef RTOS_SDK
/*
 * configTICK_RATE_HZ is 100, implying 10 ms ticks.
 * But we run CPU at 160 and tick timer is not updated, hence / 2 below.
 * https://github.com/espressif/ESP8266_RTOS_SDK/issues/90
 */
#define USECS_PER_TICK (1000000 / configTICK_RATE_HZ / 2)
  uint32_t ticks = usecs / USECS_PER_TICK;
  usecs = usecs % USECS_PER_TICK;
  if (ticks > 0) vTaskDelay(ticks);
#endif
  os_delay_us(usecs);
}

IRAM void mgos_ints_disable(void) {
  __asm volatile("rsil a2, 3" : : : "a2");
}

IRAM void mgos_ints_enable(void) {
  __asm volatile("rsil a2, 0" : : : "a2");
}

#ifdef RTOS_SDK
extern xSemaphoreHandle s_mtx;

IRAM void mgos_lock(void) {
  while (!xSemaphoreTakeRecursive(s_mtx, 10)) {
  }
}

IRAM void mgos_unlock(void) {
  while (!xSemaphoreGiveRecursive(s_mtx)) {
  }
}

#else /* !RTOS_SDK */

IRAM void mgos_lock(void) {
}

IRAM void mgos_unlock(void) {
}

#endif
