/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SJ_ADC_H_INCLUDED
#define SJ_ADC_H_INCLUDED

/* HAL */

uint32_t sj_adc_read(int pin);

/* return the voltage read by the ADC */
double sj_adc_read_voltage(int pin);

#endif
