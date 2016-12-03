#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

esp_err_t event_handler(void *ctx, system_event_t *event) {
  ESP_LOGI("foo", "event: %d", event->event_id);
  return ESP_OK;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  wifi_config_t wifi_config = {
    .ap = {
      .ssid = "Mongoose",
      .password = "Mongoose",
    }
  };
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
  int level = 0;
  while (true) {
    gpio_set_level(GPIO_NUM_4, level);
    level = !level;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI("foo", "%s", (level ? "ping" : "pong"));
  }
}

