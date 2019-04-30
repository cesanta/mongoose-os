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

#ifndef CS_MOS_LIBS_ADC_ESP8266_SRC_ESP_ADC_H_
#define CS_MOS_LIBS_ADC_ESP8266_SRC_ESP_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

int esp_adc_value_at_boot(void);

void esp_adc_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_ADC_ESP8266_SRC_ESP_ADC_H_ */
