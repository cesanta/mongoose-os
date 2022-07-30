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

#include "hal/wdt_hal.h"
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

bool g_system_restart_sys_reset = false;

void mgos_dev_system_restart(void) {
  if (g_system_restart_sys_reset) {
    // Cause system reset through WDT.
    wdt_hal_context_t rtc_wdt_ctx;
    wdt_hal_init(&rtc_wdt_ctx, WDT_RWDT, 0, false);
    wdt_hal_write_protect_disable(&rtc_wdt_ctx);
    wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE0, 1, WDT_STAGE_ACTION_RESET_SYSTEM);
    wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE1, 10, WDT_STAGE_ACTION_RESET_RTC);
    wdt_hal_set_flashboot_en(&rtc_wdt_ctx, true);
    wdt_hal_write_protect_enable(&rtc_wdt_ctx);
    wdt_hal_enable(&rtc_wdt_ctx);
    while (true) {
    }
  }
  esp_restart();
}

void device_get_mac_address(uint8_t mac[6]) {
  esp_base_mac_addr_get(mac);
}

void device_set_mac_address(uint8_t mac[6]) {
  esp_base_mac_addr_set(mac);
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
  esp_task_wdt_init(secs, true /* panic */);
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
  rtc_cpu_freq_config_t c;
  rtc_clk_cpu_freq_get_config(&c);
  return c.freq_mhz * 1000000;
}
