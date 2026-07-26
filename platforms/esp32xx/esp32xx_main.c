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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esp_attr.h"
#include "esp_debug_helpers.h"
#include "esp_event.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_mac.h"
#include "esp_rom_serial_output.h"
#else
#include "esp_spi_flash.h"
#endif
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "soc/efuse_reg.h"

#include "common/cs_dbg.h"
#include "mgos_core_dump.h"
#include "mgos_debug_internal.h"
#include "mgos_event.h"
#include "mgos_freertos.h"
#include "mgos_net_hal.h"
#include "mgos_system.h"
#include "mgos_vfs_internal.h"

#include "esp32xx_debug.h"
#ifdef MGOS_HAVE_OTA_COMMON
#include "esp32xx_updater.h"
#endif

#if defined(MGOS_ESP32)
#include "esp32/rom/ets_sys.h"
#include "esp32/rom/spi_flash.h"
#elif defined(MGOS_ESP32C3)
#include "esp32c3/rom/ets_sys.h"
#include "esp32c3/rom/spi_flash.h"
#elif defined(MGOS_ESP32C6)
#include "esp32c6/rom/ets_sys.h"
#include "esp32c6/rom/spi_flash.h"
#endif

enum mgos_init_result mgos_freertos_pre_init(void) {
  enum mgos_init_result r;

  srand(esp_random()); /* esp_random() uses HW RNG */

#ifdef MGOS_HAVE_OTA_COMMON
  esp32xx_updater_early_init();
#endif

  r = esp32xx_debug_init();
  if (r != MGOS_INIT_OK) return r;

  LOG(LL_INFO, ("ESP-IDF %s", esp_get_idf_version()));
  LOG(LL_INFO,
      ("Boot partition: %s; flash: %uM", esp_ota_get_running_partition()->label,
       (unsigned int) (g_rom_flashchip.chip_size / 1048576)));

  /*
   * IDF 5+ already chooses the base MAC according to
   * CONFIG_ESP_MAC_USE_CUSTOM_MAC_AS_BASE_MAC. Calling
   * esp_efuse_mac_get_custom() directly logs an error on chips without a
   * provisioned custom MAC.
   */
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  {
    uint8_t mac[6];
    /* Use custom MAC as base if it's configured, default otherwise. */
    if (esp_efuse_mac_get_custom(mac) != ESP_OK) {
      esp_efuse_mac_get_default(mac);
    }
    esp_base_mac_addr_set(mac);
  }
#endif

  /* Disable WDT on idle task(s), mgos task WDT should do fine. */
  for (int i = 0; i < configNUM_CORES; i++) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    TaskHandle_t h = xTaskGetIdleTaskHandleForCore(i);
#else
    TaskHandle_t h = xTaskGetIdleTaskHandleForCPU(i);
#endif
    if (h != NULL) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
      if (esp_task_wdt_status(h) == ESP_OK) {
        esp_task_wdt_delete(h);
      }
#else
      esp_task_wdt_delete(h);
#endif
    }
  }

#ifdef CS_MMAP
  mgos_vfs_mmap_init();
#endif

  return MGOS_INIT_OK;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
/* panic_print_char became static in later IDFs; the ROM putc is equivalent
 * for the console channel. */
#define mgos_panic_putc esp_rom_output_putc
#else
extern void panic_print_char(char c);
#define mgos_panic_putc panic_print_char
#endif

/*
 * Note that this function may be invoked from a very low level.
 * This is where ESP_EARLY_LOG prints (via ets_printf).
 */
static IRAM void sdk_putc(char c) {
  if (mgos_debug_uart_is_suspended()) return;
  mgos_panic_putc(c);
}

IRAM void mgos_cd_putc(int c) {
  mgos_panic_putc(c);
}

extern void cs_log_lock(void);
extern void cs_log_unlock(void);
extern FILE *cs_log_file;
extern volatile enum cs_log_level cs_log_cur_msg_level;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
/* Upstream esp_log_set_vprintf callback has no level argument
 * (the level-aware variant was a Cesanta fork patch on IDF 4.4). */
static int sdk_debug_vprintf(const char *fmt, va_list ap) {
  FILE *f = (cs_log_file ? cs_log_file : stderr);
  cs_log_lock();
  cs_log_cur_msg_level = LL_INFO;
  int res = vfprintf(f, fmt, ap);
  cs_log_cur_msg_level = LL_NONE;
  cs_log_unlock();
  return res;
}
#else
static int sdk_debug_vprintf(esp_log_level_t level, const char *fmt,
                             va_list ap) {
  FILE *f = (cs_log_file ? cs_log_file : stderr);
  enum cs_log_level cs_level = LL_NONE;
  switch (level) {
    case ESP_LOG_NONE:
      cs_level = LL_NONE;
      break;
    case ESP_LOG_ERROR:
      cs_level = LL_ERROR;
      break;
    case ESP_LOG_WARN:
      cs_level = LL_WARN;
      break;
    case ESP_LOG_INFO:
      cs_level = LL_INFO;
      break;
    case ESP_LOG_DEBUG:
      cs_level = LL_DEBUG;
      break;
    case ESP_LOG_VERBOSE:
      cs_level = LL_VERBOSE_DEBUG;
      break;
  }
  cs_log_lock();
  cs_log_cur_msg_level = cs_level;
  int res = vfprintf(f, fmt, ap);
  cs_log_cur_msg_level = LL_NONE;
  cs_log_unlock();
  return res;
}
#endif /* ESP_IDF_VERSION */

void app_main(void) {
  nvs_flash_init();
  ets_install_putc1(sdk_putc);
  ets_install_putc2(NULL);
  esp_log_set_vprintf(sdk_debug_vprintf);
  esp_event_loop_create_default();
  esp_netif_init();
  /* Scheduler is already running at this point */
  mgos_freertos_run_mgos_task(false /* start_scheduler */);
  /* Unlike other platforms, we return and abandon this task. */
}
