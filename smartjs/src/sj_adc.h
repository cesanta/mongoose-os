/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_ADC_H_
#define CS_SMARTJS_SRC_SJ_ADC_H_

/* HAL */

uint32_t sj_adc_read(int pin);

/* return the voltage read by the ADC */
double sj_adc_read_voltage(int pin);

#endif /* CS_SMARTJS_SRC_SJ_ADC_H_ */
