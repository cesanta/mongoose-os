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

/*
 * NB: the file is named esp32_hw_timers.h because mgos_hw_timers_hal.h
 * includes it under that name for all ESP32-family targets. On the C6
 * the legacy timer group driver is gone, so the dev data holds a gptimer
 * handle instead of raw timer group registers.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_ESP32_HW_TIMER_IRAM 0x10000

struct mgos_hw_timer_dev_data {
  gptimer_handle_t th;
  bool started;
};

#ifdef __cplusplus
}
#endif
