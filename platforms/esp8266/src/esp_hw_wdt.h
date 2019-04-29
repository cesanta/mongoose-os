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

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_HW_WDT_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_HW_WDT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_HW_WDT_H_ */
