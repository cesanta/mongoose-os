/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_WDT_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_WDT_H_

#include <stdint.h>

enum esp_hw_wdt_timeout {
  ESP_HW_WDT_DISABLE = 0,
  ESP_HW_WDT_0_84_SEC = 10,
  ESP_HW_WDT_1_68_SEC = 11,
  ESP_HW_WDT_3_36_SEC = 12,
  ESP_HW_WDT_6_71_SEC = 13,
  ESP_HW_WDT_13_4_SEC = 14,
  ESP_HW_WDT_26_8_SEC = 15,
};

void esp_hw_wdt_setup(enum esp_hw_wdt_timeout stage0_timeout,
                      enum esp_hw_wdt_timeout stage1_timeout);
enum esp_hw_wdt_timeout esp_hw_wdt_secs_to_timeout(int secs);
void esp_hw_wdt_enable();
void esp_hw_wdt_disable();
void esp_hw_wdt_feed();

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_WDT_H_ */
