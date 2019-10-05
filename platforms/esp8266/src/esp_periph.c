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

/*
 * Map gpio -> { mux reg, func }
 * SDK doesn't provide information
 * about several gpio
 * TODO(alashkin): find missed info
 */

const struct gpio_info gpio_map[] = {
    {0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0},
    {1, PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1},
    {2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2},
    {3, PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3},
    {4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4},
    {5, PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5},
    {6, 0, 0},
    {7, 0, 0},
    {8, 0, 0},
    {9, PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9},
    {10, PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10},
    {11, 0, 0},
    {12, PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12},
    {13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13},
    {14, PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14},
    {15, PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15},
};

IRAM const struct gpio_info *get_gpio_info(uint8_t gpio_no) {
  const struct gpio_info *ret_val;

  if (gpio_no >= sizeof(gpio_map) / sizeof(gpio_map[0])) {
    return NULL;
  }

  ret_val = &gpio_map[gpio_no];

  if (ret_val->periph == 0) {
    /* unknown gpio */
    return NULL;
  }

  return ret_val;
}

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
