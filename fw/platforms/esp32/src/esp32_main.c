/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"

#include "fw/platforms/esp32/src/esp32_console.h"
#include "fw/platforms/esp32/src/esp32_fs.h"

#define MGOS_TASK_STACK_SIZE 8192
#define MGOS_TASK_QUEUE_LENGTH 32

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
  cs_log_set_level(LL_INFO);
  mongoose_init();

  r = esp32_console_init();
  if (r != MGOS_INIT_OK) return r;

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS Firmware %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("Task ID: %p, RAM: %u free", xTaskGetCurrentTaskHandle(),
                mgos_get_free_heap_size()));

  if (esp32_fs_init() != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mount FS"));
    return MGOS_INIT_FS_INIT_FAILED;
  }

  if ((r = mgos_init()) != MGOS_INIT_OK) return r;

  return MGOS_INIT_OK;
}

static QueueHandle_t s_main_queue;

void mgos_task(void *arg) {
  struct mgos_event e;
  s_main_queue = xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(e));

  mgos_app_preinit();

  enum mgos_init_result r = esp32_mgos_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("MGOS init failed: %d", r));
    abort();
  }

  while (true) {
    mongoose_poll(0);
    while (xQueueReceive(s_main_queue, &e, 1 /* tick */)) {
      e.cb(e.arg);
    }
  }

  (void) arg;
}

bool IRAM_ATTR mgos_invoke_cb(mgos_cb_t cb, void *arg) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  int should_yield = false;
  if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
    return false;
  }
  if (should_yield) {
    /*
     * TODO(rojer): Find a way to determine if we're in an interrupt and
     * invoke portYIELD or portYIELD_FROM_ISR.
     */
  }
  return true;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  TaskHandle_t th;
  xTaskCreate(mgos_task, "mgos", MGOS_TASK_STACK_SIZE, NULL, 5, &th);
}
