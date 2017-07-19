/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include "common/cs_dbg.h"
#include "mgos_adc.h"

bool mgos_adc_enable(int pin) {
  LOG(LL_ERROR, ("ADC is not implemented"));
  (void) pin;
  return false;
}

int mgos_adc_read(int pin) {
  LOG(LL_ERROR, ("ADC is not implemented"));
  (void) pin;
  return -1;
}
