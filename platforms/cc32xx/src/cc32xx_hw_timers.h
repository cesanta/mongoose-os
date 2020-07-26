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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: We take a simple approach and run timers in combined 32-bit mode.
 * It may be possible to be smarter and allocate 16 bit halves if 16-bit divider
 * is enough.
 * Something to investigate in the future.
 */

#define MGOS_ESP32_HW_TIMER_IRAM 0x10000

struct mgos_hw_timer_dev_data {
  uint32_t base;
  uint32_t periph;
  int int_no;
  void (*int_handler)(void);
};

#ifdef __cplusplus
}
#endif
