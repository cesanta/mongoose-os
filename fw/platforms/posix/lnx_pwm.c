/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#include <stdbool.h>

bool mgos_pwm_set(int pin, int freq, int duty) {
  (void) pin;
  (void) freq;
  (void) duty;

  return false;
}
