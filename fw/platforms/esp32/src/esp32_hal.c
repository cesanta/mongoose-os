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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "soc/rtc.h"

#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_sys_config.h"
#include "mgos_vfs.h"

size_t mgos_get_heap_size(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_8BIT);
  return info.total_free_bytes + info.total_allocated_bytes;
}

size_t mgos_get_free_heap_size(void) {
  return xPortGetFreeHeapSize();
}

size_t mgos_get_min_free_heap_size(void) {
  return xPortGetMinimumEverFreeHeapSize();
}

void mgos_dev_system_restart(void) {
  esp_restart();
}

void device_get_mac_address(uint8_t mac[6]) {
  esp_efuse_mac_get_default(mac);
}

void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

IRAM void mgos_usleep(uint32_t usecs) {
  uint64_t threshold = esp_timer_get_time() + (uint64_t) usecs;
  int ticks = usecs / (1000000 / configTICK_RATE_HZ);
  if (ticks > 0) vTaskDelay(ticks);
  while (esp_timer_get_time() < threshold) {
  }
}

void mgos_wdt_feed(void) {
  esp_task_wdt_reset();
}

void mgos_wdt_disable(void) {
  esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
}

void mgos_wdt_enable(void) {
  esp_task_wdt_add(xTaskGetCurrentTaskHandle());
}

void mgos_wdt_set_timeout(int secs) {
  /* WDT0 is configured in esp_task_wdt_init, we only modify the timeout. */
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_config2 = secs * 2000; /* Units: 0.5 ms ticks */
  TIMERG0.wdt_config3 = (secs + 1) * 2000;
  TIMERG0.wdt_config0.en = 1;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  while (len > 0) {
    uint32_t r = esp_random(); /* Uses hardware RNG. */
    for (int i = 0; i < 4 && len > 0; i++, len--) {
      *buf++ = (uint8_t) r;
      r >>= 8;
    }
  }
  (void) ctx;
  return 0;
}

uint32_t mgos_get_cpu_freq(void) {
  return rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get());
}
