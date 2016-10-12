/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef MG_ENABLE_ADC_API

#include <stdint.h>

uint32_t mg_adc_read(int pin) {
  (void) pin;
  return 0;
}

double mg_adc_read_voltage(int pin) {
  return mg_adc_read(pin);
}

#endif /* MG_ENABLE_ADC_API */
