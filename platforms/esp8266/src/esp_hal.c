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

#include "esp_missing_includes.h"

#ifdef RTOS_SDK
#include "esp_misc.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include <os_type.h>
#include <osapi.h>
#include <user_interface.h>
#endif

#include "common/cs_dbg.h"
#include "umm_malloc.h"

#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_time.h"
#include "mgos_timers.h"

#include "esp_hw_wdt.h"

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
  /* Keep the soft WDT stopped. */
  system_soft_wdt_stop();
}

void mgos_wdt_set_timeout(int secs) {
  esp_hw_wdt_setup(esp_hw_wdt_secs_to_timeout(secs), ESP_HW_WDT_1_68_SEC);
}

void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

static uint8_t prev_intlevel = 0, nest_level = 0;

IRAM void mgos_ints_disable(void) {
  uint32_t prev_ps;
  asm volatile("rsil %0, 3 \n" : "=a"(prev_ps) : :);
  if (nest_level == 0) {
    prev_intlevel = (prev_ps & 0xf);
  }
  nest_level++;
}

IRAM void mgos_ints_enable(void) {
  nest_level--;
  if (nest_level != 0) return;
  switch (prev_intlevel) {
    case 0:
      asm volatile("rsil a2, 0" : : : "a2");
      break;
    case 1:
      asm volatile("rsil a2, 1" : : : "a2");
      break;
    case 2:
      asm volatile("rsil a2, 2" : : : "a2");
      break;
    default:
      asm volatile("rsil a2, 3" : : : "a2");
  }
}

#ifdef RTOS_SDK
extern xSemaphoreHandle s_mtx;

IRAM void mgos_lock(void) {
  xSemaphoreTakeRecursive(s_mtx, portMAX_DELAY);
}

IRAM void mgos_unlock(void) {
  xSemaphoreGiveRecursive(s_mtx);
}

struct mgos_rlock_type *mgos_rlock_create(void) {
  return (struct mgos_rlock_type *) xSemaphoreCreateRecursiveMutex();
}

IRAM void mgos_rlock(struct mgos_rlock_type *l) {
  xSemaphoreTakeRecursive((xSemaphoreHandle) l, portMAX_DELAY);
}

IRAM void mgos_runlock(struct mgos_rlock_type *l) {
  xSemaphoreGiveRecursive((xSemaphoreHandle) l);
}

IRAM void mgos_rlock_destroy(struct mgos_rlock_type *l) {
  vSemaphoreDelete((xSemaphoreHandle) l);
}
#else /* !RTOS_SDK */

IRAM void mgos_lock(void) {
}

IRAM void mgos_unlock(void) {
}

struct mgos_rlock_type *mgos_rlock_create(void) {
  return NULL;
}

IRAM void mgos_rlock(struct mgos_rlock_type *l) {
  (void) l;
}

IRAM void mgos_runlock(struct mgos_rlock_type *l) {
  (void) l;
}

IRAM void mgos_rlock_destroy(struct mgos_rlock_type *l) {
  (void) l;
}

#endif /* !RTOS_SDK */

uint32_t mgos_get_cpu_freq(void) {
  return system_get_cpu_freq() * 1000000;
}

// For LwIP
uint32_t sys_now(void) {
  return mgos_uptime_micros() / 1000;
}

// os_get_random uses hardware RNG, so it's cool.
int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) WEAK;
int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  os_get_random(buf, len);
  (void) ctx;
  return 0;
}
