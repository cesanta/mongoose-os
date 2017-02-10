/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_features.h"

#include <stdint.h>

uint32_t mgos_adc_read(int pin) {
  (void) pin;
  return 0;
}

double mgos_adc_read_voltage(int pin) {
  return mgos_adc_read(pin);
}
