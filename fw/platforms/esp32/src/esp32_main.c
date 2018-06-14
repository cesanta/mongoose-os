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
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_panic.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "rom/spi_flash.h"

#include "common/cs_dbg.h"
#include "mgos_core_dump.h"
#include "mgos_debug_internal.h"
#include "mgos_event.h"
#include "mgos_hal_freertos_internal.h"
#include "mgos_net_hal.h"
#include "mgos_system.h"
#include "mgos_vfs_internal.h"
#ifdef MGOS_HAVE_WIFI
#include "esp32_wifi.h"
#endif

#include "esp32_debug.h"
#include "esp32_exc.h"
#ifdef MGOS_HAVE_OTA_COMMON
#include "esp32_updater.h"
#endif

esp_err_t esp32_wifi_ev(system_event_t *event);

esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
#ifdef MGOS_HAVE_WIFI
    case SYSTEM_EVENT_STA_START:
    case SYSTEM_EVENT_STA_STOP:
    case SYSTEM_EVENT_STA_GOT_IP:
    case SYSTEM_EVENT_STA_CONNECTED:
    case SYSTEM_EVENT_STA_DISCONNECTED:
    case SYSTEM_EVENT_AP_START:
    case SYSTEM_EVENT_AP_STOP:
    case SYSTEM_EVENT_AP_STACONNECTED:
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    case SYSTEM_EVENT_SCAN_DONE:
      return esp32_wifi_ev(event);
      break;
#endif
#ifdef MGOS_HAVE_ETHERNET
    case SYSTEM_EVENT_ETH_START:
    case SYSTEM_EVENT_ETH_STOP:
      break;
    case SYSTEM_EVENT_ETH_CONNECTED: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_CONNECTED);
      break;
    }
    case SYSTEM_EVENT_ETH_DISCONNECTED: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_DISCONNECTED);
      break;
    }
    case SYSTEM_EVENT_ETH_GOT_IP: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_IP_ACQUIRED);
      break;
    }
#endif
    default:
      LOG(LL_INFO, ("event: %d", event->event_id));
  }
  return ESP_OK;
}

enum mgos_init_result mgos_hal_freertos_pre_init(void) {
  enum mgos_init_result r;

  srand(esp_random()); /* esp_random() uses HW RNG */

#ifdef MGOS_HAVE_OTA_COMMON
  esp32_updater_early_init();
#endif

  r = esp32_debug_init();
  if (r != MGOS_INIT_OK) return r;

  LOG(LL_INFO, ("ESP-IDF %s", esp_get_idf_version()));
  LOG(LL_INFO,
      ("Boot partition: %s; flash: %uM", esp_ota_get_boot_partition()->label,
       g_rom_flashchip.chip_size / 1048576));

  /* Use default mac as base. Makes no difference but silences warnings. */
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  esp_base_mac_addr_set(mac);

  /* Disable WDT on idle task(s), mgos task WDT should do fine. */
  TaskHandle_t h;
  if ((h = xTaskGetIdleTaskHandleForCPU(0)) != NULL) esp_task_wdt_delete(h);
  if ((h = xTaskGetIdleTaskHandleForCPU(1)) != NULL) esp_task_wdt_delete(h);

#ifdef CS_MMAP
  mgos_vfs_mmap_init();
#endif

  esp32_exception_handler_init();

  return MGOS_INIT_OK;
}

/*
 * Note that this function may be invoked from a very low level.
 * This is where ESP_EARLY_LOG prints (via ets_printf).
 */
static IRAM void sdk_putc(char c) {
  if (mgos_debug_uart_is_suspended()) return;
  ets_write_char_uart(c);
}

void mgos_cd_putc(int c) {
  panicPutChar(c);
}

extern enum cs_log_level cs_log_cur_msg_level;
static int sdk_debug_vprintf(const char *fmt, va_list ap) {
  /* Do not log SDK messages anywhere except UART. */
  cs_log_cur_msg_level = LL_VERBOSE_DEBUG;
  int res = vprintf(fmt, ap);
  cs_log_cur_msg_level = LL_NONE;
  return res;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ets_install_putc1(sdk_putc);
  ets_install_putc2(NULL);
  esp_log_set_vprintf(sdk_debug_vprintf);
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  /* Scheduler is already running at this point */
  mgos_hal_freertos_run_mgos_task(false /* start_scheduler */);
  /* Unlike other platforms, we return and abandon this task. */
}
