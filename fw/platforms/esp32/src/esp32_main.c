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
#include "fw/src/miot_hal.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/platforms/esp32/src/esp32_fs.h"

#define MIOT_TASK_STACK_SIZE 8192
#define MIOT_TASK_QUEUE_LENGTH 32

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

/* From esp32_wifi.c */
esp_err_t wifi_event_handler(system_event_t *event);
bool miot_wifi_set_config(const struct sys_config_wifi *cfg);

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

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  /* TODO: UART settings */
  return miot_wifi_set_config(&cfg->wifi) ? MIOT_INIT_OK
                                          : MIOT_INIT_CONFIG_WIFI_INIT_FAILED;
}

struct miot_event {
  miot_cb_t cb;
  void *arg;
};

QueueHandle_t s_main_queue;

void miot_task(void *arg) {
  /* Enable WDT for this task. It will be fed by Mongoose polling loop. */
  esp_task_wdt_feed();
  cs_log_set_level(LL_INFO);
  mongoose_init();

  if (strcmp(MIOT_APP, "mongoose-iot") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MIOT_APP, build_version, build_id));
  }
  LOG(LL_INFO,
      ("Mongoose IoT Firmware %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("Task ID: %p, RAM: %u free", xTaskGetCurrentTaskHandle(),
                miot_get_free_heap_size()));

  if (esp32_fs_init() != MIOT_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mount FS"));
    abort();
  }

  enum miot_init_result r = mg_init();
  if (r != MIOT_INIT_OK) {
    LOG(LL_ERROR, ("MIOT init failed: %d", r));
    abort();
  }

  struct miot_event e;
  s_main_queue = xQueueCreate(MIOT_TASK_QUEUE_LENGTH, sizeof(e));

  while (true) {
    mongoose_poll(0);
    while (xQueueReceive(s_main_queue, &e, 1 /* tick */)) {
      e.cb(e.arg);
    }
  }

  (void) arg;
}

bool IRAM_ATTR miot_invoke_cb(miot_cb_t cb, void *arg) {
  struct miot_event e = {.cb = cb, .arg = arg};
  int should_yield = false;
  if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
    return false;
  }
  if (should_yield) vPortYield();
  return true;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  TaskHandle_t th;
  xTaskCreate(miot_task, "miot", MIOT_TASK_STACK_SIZE, NULL, 5, &th);
}
