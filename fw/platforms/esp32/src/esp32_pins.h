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

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_PINS_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_PINS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Pin to GPIO mappings for ESP-WROOM-32 */

/* Pin 1: GND */
/* Pin 2: 3V3 */
/* Pin 3: EN */
#define WROOM32_PIN_4_GPIO 36
#define WROOM32_PIN_5_GPIO 39
#define WROOM32_PIN_6_GPIO 34
#define WROOM32_PIN_7_GPIO 35
#define WROOM32_PIN_8_GPIO 32
#define WROOM32_PIN_9_GPIO 33
#define WROOM32_PIN_10_GPIO 25
#define WROOM32_PIN_11_GPIO 26
#define WROOM32_PIN_12_GPIO 27
#define WROOM32_PIN_13_GPIO 14
#define WROOM32_PIN_14_GPIO 12
/* Pin 15: GND */
#define WROOM32_PIN_16_GPIO 13
/* Pins 17-22: SFLASH pins */
#define WROOM32_PIN_23_GPIO 15
#define WROOM32_PIN_24_GPIO 2
#define WROOM32_PIN_25_GPIO 0
#define WROOM32_PIN_26_GPIO 4
#define WROOM32_PIN_27_GPIO 16
#define WROOM32_PIN_28_GPIO 17
#define WROOM32_PIN_29_GPIO 5
#define WROOM32_PIN_30_GPIO 18
#define WROOM32_PIN_31_GPIO 19
/* Pin 32: NC */
#define WROOM32_PIN_33_GPIO 21
#define WROOM32_PIN_34_GPIO 3
#define WROOM32_PIN_35_GPIO 1
#define WROOM32_PIN_36_GPIO 22
#define WROOM32_PIN_37_GPIO 23
/* Pin 38: GND */

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_PINS_H_ */
