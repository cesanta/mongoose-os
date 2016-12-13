/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_ADC_H_
#define CS_FW_SRC_MIOT_ADC_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* HAL */

uint32_t miot_adc_read(int pin);

/* return the voltage read by the ADC */
double miot_adc_read_voltage(int pin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_ADC_H_ */
