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

#ifndef CS_MOS_LIBS_ADC_SRC_MGOS_ADC_H_
#define CS_MOS_LIBS_ADC_SRC_MGOS_ADC_H_

#include <stdbool.h>

#if CS_PLATFORM == CS_P_ESP32
#include "esp32/esp32_adc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Configure and enable ADC */
bool mgos_adc_enable(int pin);

/* Read from the analog pin. Returns raw value. */
int mgos_adc_read(int pin);

/*
 * Read from the specified analog pin.
 * Returns voltage on the pin, in mV.
 */
int mgos_adc_read_voltage(int pin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_ADC_SRC_MGOS_ADC_H_ */
