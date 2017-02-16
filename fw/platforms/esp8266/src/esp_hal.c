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

#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"

#include "fw/src/mgos_mongoose.h"
#include "common/umm_malloc/umm_malloc.h"

#include "fw/platforms/esp8266/src/esp_fs.h"

size_t mgos_get_heap_size(void) {
  return UMM_MALLOC_CFG__HEAP_SIZE;
}

size_t mgos_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t mgos_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

#ifdef RTOS_SDK
extern xSemaphoreHandle s_mtx;

void mgos_wdt_disable(void) {
  /* TODO(rojer) */
}

void mgos_wdt_enable(void) {
  /* TODO(rojer) */
}

void mgos_wdt_feed(void) {
  system_soft_wdt_feed();
}

void mgos_wdt_set_timeout(int secs) {
  /* TODO(rojer) */
  (void) secs;
}

IRAM void mgos_lock(void) {
  while (!xSemaphoreTakeRecursive(s_mtx, 10)) {
  }
}

IRAM void mgos_unlock(void) {
  while (!xSemaphoreGiveRecursive(s_mtx)) {
  }
}

#else /* !RTOS_SDK */

extern uint32_t soft_wdt_interval;
/* Should be initialized in user_main by calling mgos_wdt_set_timeout */
static uint32_t s_saved_soft_wdt_interval;
#define WDT_MAGIC_TIME 500000

void mgos_wdt_disable(void) {
  /*
   * poor's man version: delays wdt for several hours, but
   * technically wdt is not disabled
   */
  s_saved_soft_wdt_interval = soft_wdt_interval;
  soft_wdt_interval = 0xFFFFFFFF;
  system_soft_wdt_restart();
}

void mgos_wdt_enable(void) {
  soft_wdt_interval = s_saved_soft_wdt_interval;
  system_soft_wdt_restart();
}

void mgos_wdt_feed(void) {
  system_soft_wdt_feed();
}

void mgos_wdt_set_timeout(int secs) {
  s_saved_soft_wdt_interval = soft_wdt_interval = secs * WDT_MAGIC_TIME;
  system_soft_wdt_restart();
}

IRAM void mgos_lock(void) {
}

IRAM void mgos_unlock(void) {
}
#endif

void mgos_system_restart(int exit_code) {
  (void) exit_code;
  fs_umount();
  system_restart();
}

void mgos_usleep(int usecs) {
#ifdef RTOS_SDK
  int ticks = usecs / (1000000 / configTICK_RATE_HZ);
  usecs = usecs % (1000000 / configTICK_RATE_HZ);
  if (ticks > 0) vTaskDelay(ticks);
#endif
  os_delay_us(usecs);
}
