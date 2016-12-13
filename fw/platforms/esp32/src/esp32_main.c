/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "common/cs_dbg.h"
#include "fw/platforms/esp32/src/esp32_fs.h"

#define MIOT_TASK_STACK_SIZE 8192

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

esp_err_t event_handler(void *ctx, system_event_t *event) {
  ESP_LOGI("foo", "event: %d", event->event_id);
  return ESP_OK;
}

void miot_task(void *arg) {
  cs_log_set_level(LL_DEBUG);
  if (strcmp(MIOT_APP, "mongoose-iot") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MIOT_APP, build_version, build_id));
  }
  LOG(LL_INFO,
      ("Mongoose IoT Firmware %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("Task ID: %p, RAM: %d free", xTaskGetCurrentTaskHandle(),
                esp_get_free_heap_size()));

  if (esp32_fs_init() != MIOT_INIT_OK) {
    ESP_LOGE("fs", "Failed to mount FS");
    abort();
  }

  FILE *f = fopen("test.txt", "r");
  if (f != NULL) {
    char buf[100];
    int n = fread(buf, 1, sizeof(buf), f);
    if (n > 0) {
      LOG(LL_INFO, ("f:\n%.*s", n, buf));
    }
    fclose(f);
  }

  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  (void) arg;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  TaskHandle_t th;
  xTaskCreate(miot_task, "miot", MIOT_TASK_STACK_SIZE, NULL, 5, &th);
}
