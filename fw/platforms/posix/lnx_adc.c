/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef SJ_ENABLE_ADC_API

#include <stdint.h>

uint32_t sj_adc_read(int pin) {
  (void) pin;
  return 0;
}

double sj_adc_read_voltage(int pin) {
  return sj_adc_read(pin);
}

#endif /* SJ_ENABLE_ADC_API */
