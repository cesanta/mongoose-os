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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_intr_alloc.h"
#include "soc/timer_group_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_ESP32_HW_TIMER_IRAM 0x10000

struct mgos_hw_timer_dev_data {
  intr_handle_t inth;
  timg_dev_t *tg;
  uint8_t tgn;
  uint8_t tn;
  bool iram;
};

#ifdef __cplusplus
}
#endif
