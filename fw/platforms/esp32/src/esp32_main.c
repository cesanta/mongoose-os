/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "fw/platforms/esp32/src/esp32_fs.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

esp_err_t event_handler(void *ctx, system_event_t *event) {
  ESP_LOGI("foo", "event: %d", event->event_id);
  return ESP_OK;
}

void app_main(void) {
  if (strcmp(MIOT_APP, "mongoose-iot") != 0) {
    ESP_LOGI("miot", "%s %s (%s)", MIOT_APP, build_version, build_id);
  }
  ESP_LOGI("miot", "Mongoose IoT Firmware %s (%s)", mg_build_version,
           mg_build_id);
  ESP_LOGI("miot", "RAM: %d free", esp_get_free_heap_size());

  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  wifi_config_t wifi_config = {
      .ap = {
          .ssid = "Mongoose", .password = "Mongoose", }};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  if (esp32_fs_init() != MIOT_INIT_OK) {
    ESP_LOGE("fs", "Failed to mount FS");
    abort();
  }

  gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
  int level = 0;
  while (true) {
    gpio_set_level(GPIO_NUM_4, level);
    level = !level;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI("foo", "%s", (level ? "ping" : "pong"));
  }
}
