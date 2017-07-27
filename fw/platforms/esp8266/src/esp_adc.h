/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_ADC_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

int esp_adc_value_at_boot(void);

void esp_adc_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_ADC_H_ */
