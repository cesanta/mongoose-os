/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_vfs.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_debug.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_updater_common.h"

#include "fw/platforms/esp32/src/esp32_debug.h"
#include "fw/platforms/esp32/src/esp32_fs.h"
#include "fw/platforms/esp32/src/esp32_updater.h"

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* in bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

/* From esp32_wifi.c */
esp_err_t wifi_event_handler(system_event_t *event);
bool mgos_wifi_set_config(const struct sys_config_wifi *cfg);

esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
      /* https://github.com/espressif/esp-idf/issues/161 */
      return wifi_event_handler(event);
    case SYSTEM_EVENT_STA_CONNECTED:
    case SYSTEM_EVENT_STA_DISCONNECTED:
    case SYSTEM_EVENT_AP_STACONNECTED:
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      break;
    default:
      LOG(LL_INFO, ("event: %d", event->event_id));
  }
  return ESP_OK;
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
  return mgos_wifi_set_config(&cfg->wifi) ? MGOS_INIT_OK
                                          : MGOS_INIT_CONFIG_WIFI_INIT_FAILED;
}

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

static enum mgos_init_result esp32_mgos_init() {
  enum mgos_init_result r;

  /* Enable WDT for this task. It will be fed by Mongoose polling loop. */
  esp_task_wdt_feed();

#if MGOS_ENABLE_UPDATER
  esp32_updater_early_init();
#endif

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);
  mongoose_init();

  r = esp32_debug_init();
  if (r != MGOS_INIT_OK) return r;
  r = mgos_debug_uart_init();
  if (r != MGOS_INIT_OK) return r;

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS Firmware %s (%s)%s", mg_build_version, mg_build_id,
#if MGOS_ENABLE_UPDATER
                (esp32_is_first_boot() ? ", first boot" : "")
#else
                ""
#endif
                    ));
  LOG(LL_INFO, ("ESP-IDF %s", esp_get_idf_version()));
  LOG(LL_INFO, ("Boot partition: %s, Task ID: %p, RAM: %u free",
                esp_ota_get_boot_partition()->label,
                xTaskGetCurrentTaskHandle(), mgos_get_free_heap_size()));

  if (esp32_fs_init() != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mount FS"));
    return MGOS_INIT_FS_INIT_FAILED;
  }

  mgos_wdt_feed();

#if MGOS_ENABLE_UPDATER
  if (esp32_is_first_boot() && mgos_upd_apply_update() < 0) {
    return MGOS_INIT_APPLY_UPDATE_FAILED;
  }
#endif

  if ((r = mgos_init()) != MGOS_INIT_OK) return r;

  return MGOS_INIT_OK;
}

extern SemaphoreHandle_t s_mgos_mux;
static QueueHandle_t s_main_queue;

void mgos_task(void *arg) {
  struct mgos_event e;
  s_main_queue = xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(e));

  mgos_app_preinit();

  enum mgos_init_result r = esp32_mgos_init();
  bool success = (r == MGOS_INIT_OK);
  if (!success) LOG(LL_ERROR, ("MGOS init failed: %d", r));

#if MGOS_ENABLE_UPDATER
  mgos_upd_boot_finish(success, esp32_is_first_boot());
#endif

  if (!success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart(0);
  }

  while (true) {
    mongoose_poll(0);
    while (xQueueReceive(s_main_queue, &e, 1 /* tick */)) {
      e.cb(e.arg);
    }
  }

  (void) arg;
}

bool IRAM_ATTR mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  if (from_isr) {
    int should_yield = false;
    if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
      return false;
    }
    if (should_yield) {
      portYIELD_FROM_ISR();
    }
    return true;
  } else {
    return xQueueSendToBack(s_main_queue, &e, 10);
  }
}

/*
 * Note that this function may be invoked from a very low level.
 * This is where ESP_EARLY_LOG prints (via ets_printf).
 */
static IRAM void sdk_putc(char c) {
  if (mgos_debug_uart_is_suspended()) return;
  ets_write_char_uart(c);
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
  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  xTaskCreate(mgos_task, "mgos", MGOS_TASK_STACK_SIZE, NULL, MGOS_TASK_PRIORITY,
              NULL);
}
