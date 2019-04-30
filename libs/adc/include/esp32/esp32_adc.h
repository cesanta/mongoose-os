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
 * ADC API specific to the ESP32 platform.
 */

#ifndef CS_MOS_LIBS_ADC_INCLUDE_ESP32_ESP32_ADC_H_
#define CS_MOS_LIBS_ADC_INCLUDE_ESP32_ESP32_ADC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Set ADC voltage reference, used to convert raw values to voltage.
 * See
 * https://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/adc.html#adc-calibration
 * for more details.
 * By default, 1.1V Vref is assumed, i.e. perfectly accurate.
 * Must be called before any ADC channels are configured.
 */
void esp32_adc_set_vref(int vref_mv);

void esp32_adc_set_width(int width);
bool esp32_set_channel_attenuation(int pin, int atten);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_ADC_INCLUDE_ESP32_ESP32_ADC_H_ */
