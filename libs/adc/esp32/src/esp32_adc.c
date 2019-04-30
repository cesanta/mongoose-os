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

#include "esp32/esp32_adc.h"

#include <stdbool.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "common/platform.h"

#include "mgos_adc.h"
#include "mgos_sys_config.h"

static int s_vref = 1100;
static int s_width = ADC_WIDTH_12Bit;

struct esp32_adc_channel_info {
  int pin;
  adc1_channel_t ch;
  adc_atten_t atten;
  esp_adc_cal_characteristics_t chars;
};

static struct esp32_adc_channel_info s_chans[8] = {
    {.pin = 36, .ch = ADC1_CHANNEL_0, .atten = ADC_ATTEN_DB_11},
    {.pin = 37, .ch = ADC1_CHANNEL_1, .atten = ADC_ATTEN_DB_11},
    {.pin = 38, .ch = ADC1_CHANNEL_2, .atten = ADC_ATTEN_DB_11},
    {.pin = 39, .ch = ADC1_CHANNEL_3, .atten = ADC_ATTEN_DB_11},
    {.pin = 32, .ch = ADC1_CHANNEL_4, .atten = ADC_ATTEN_DB_11},
    {.pin = 33, .ch = ADC1_CHANNEL_5, .atten = ADC_ATTEN_DB_11},
    {.pin = 34, .ch = ADC1_CHANNEL_6, .atten = ADC_ATTEN_DB_11},
    {.pin = 35, .ch = ADC1_CHANNEL_7, .atten = ADC_ATTEN_DB_11},
};

static struct esp32_adc_channel_info *esp32_adc_get_channel_info(int pin) {
  for (int i = 0; i < ARRAY_SIZE(s_chans); i++) {
    if (s_chans[i].pin == pin) return &s_chans[i];
  }
  return NULL;
}

static bool esp32_update_channel_settings(struct esp32_adc_channel_info *ci) {
  if (adc1_config_channel_atten(ci->ch, ci->atten) != ESP_OK) {
    return false;
  }

  esp_adc_cal_characterize(ADC_UNIT_1, ci->atten, s_width, s_vref, &ci->chars);
  return true;
}

bool esp32_set_channel_attenuation(int pin, int atten) {
  struct esp32_adc_channel_info *ci = esp32_adc_get_channel_info(pin);
  if (ci == NULL) return false;

  ci->atten = (adc_atten_t) atten;
  return true;
}

bool mgos_adc_enable(int pin) {
  struct esp32_adc_channel_info *ci = esp32_adc_get_channel_info(pin);
  if (ci == NULL) return false;

  return esp32_update_channel_settings(ci);
}

int mgos_adc_read(int pin) {
  struct esp32_adc_channel_info *ci = esp32_adc_get_channel_info(pin);
  if (ci == NULL) return false;
  return adc1_get_raw(ci->ch);
}

int mgos_adc_read_voltage(int pin) {
  struct esp32_adc_channel_info *ci = esp32_adc_get_channel_info(pin);
  if (ci == NULL) return false;
  uint32_t voltage;
  esp_adc_cal_get_voltage(ci->ch, &ci->chars, &voltage);
  return voltage;
}

void esp32_adc_set_vref(int vref_mv) {
  s_vref = vref_mv;
}

void esp32_adc_set_width(int width) {
  if ((width >= ADC_WIDTH_BIT_9) && (width <= ADC_WIDTH_BIT_12)) {
    s_width = width;
  }
  adc1_config_width(s_width);
}

bool mgos_adc_init(void) {
  if (mgos_sys_config_get_sys_esp32_adc_vref() > 0) {
    esp32_adc_set_vref(mgos_sys_config_get_sys_esp32_adc_vref());
  }
  esp32_adc_set_width(mgos_sys_config_get_sys_esp32_adc_width());
  return true;
}
