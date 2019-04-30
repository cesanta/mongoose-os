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

#include "esp_adc.h"

#include <stdlib.h>

#ifdef RTOS_SDK
#include <esp_system.h>
#else
#include <user_interface.h>
#endif

bool mgos_adc_enable(int pin) {
  return pin == 0;
}

int mgos_adc_read(int pin) {
  int16_t res;
#if MGOS_ADC_MODE_VDD
  res = system_get_vdd33();
#else
  res = system_adc_read();
#endif
  return pin == 0 ? 0xFFFF & res : -1;
}

static int s_adc_at_boot = 0;

int esp_adc_value_at_boot(void) {
  return s_adc_at_boot;
}

void esp_adc_init(void) {
  s_adc_at_boot = mgos_adc_read(0);
}

bool mgos_adc_init(void) {
  srand(rand() ^ s_adc_at_boot);
  return true;
}
