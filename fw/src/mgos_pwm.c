/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * PWM API wrapper for a ffi
 */

#include <stdbool.h>
#include "fw/src/mgos_pwm.h"

bool mgos_pwm_set_double(int pin, int freq, double duty) {
  return mgos_pwm_set(pin, freq, duty);
}
