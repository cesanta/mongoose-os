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

#include "mgos_gpio_hal.h"

/* TODO(dfrank) */

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  return false;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  return false;
}

bool mgos_gpio_read(int pin) {
  return false;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  return MGOS_INIT_OK;
}
