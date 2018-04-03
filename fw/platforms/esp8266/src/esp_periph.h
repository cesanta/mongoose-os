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

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_PERIPH_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_PERIPH_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_info {
  uint8_t pin;
  uint32_t periph;
  uint32_t func;
};

const struct gpio_info *get_gpio_info(uint8_t gpio_no);

enum esp_chip_type {
  ESP_CHIP_TYPE_ESP8266EX = 0,
  ESP_CHIP_TYPE_ESP8285 = 1,
};
enum esp_chip_type esp_get_chip_type(void);
const char *esp_chip_type_str(enum esp_chip_type dev_type);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_PERIPH_H_ */
