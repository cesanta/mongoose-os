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

#include "esp_periph.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef RTOS_SDK
#include <esp8266/pin_mux_register.h>
#else
#include <ets_sys.h>
#endif

enum esp_chip_type esp_get_chip_type(void) {
  uint32_t efuse0 = READ_PERI_REG(0x3ff00050);
  uint32_t efuse2 = READ_PERI_REG(0x3ff00058);
  // https://github.com/espressif/esptool/blob/200ab6e39487bef1df9db12a5be5b0682d80d3c1/esptool.py#L885
  if ((efuse0 & (1 << 4)) != 0 || (efuse2 & (1 << 16)) != 0) {
    return ESP_CHIP_TYPE_ESP8285;
  }
  return ESP_CHIP_TYPE_ESP8266EX;
}

const char *esp_chip_type_str(enum esp_chip_type dev_type) {
  switch (dev_type) {
    case ESP_CHIP_TYPE_ESP8266EX:
      return "ESP8266EX";
    case ESP_CHIP_TYPE_ESP8285:
      return "ESP8285";
  }
  return "";
}
