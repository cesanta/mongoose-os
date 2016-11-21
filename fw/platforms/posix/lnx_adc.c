/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_ADC_API

#include <stdint.h>

uint32_t miot_adc_read(int pin) {
  (void) pin;
  return 0;
}

double miot_adc_read_voltage(int pin) {
  return miot_adc_read(pin);
}

#endif /* MIOT_ENABLE_ADC_API */
