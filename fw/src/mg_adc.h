/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_ADC_H_
#define CS_FW_SRC_MG_ADC_H_

#include <inttypes.h>

/* HAL */

uint32_t mg_adc_read(int pin);

/* return the voltage read by the ADC */
double mg_adc_read_voltage(int pin);

#endif /* CS_FW_SRC_MG_ADC_H_ */
