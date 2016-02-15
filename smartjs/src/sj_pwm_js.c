/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "sj_common.h"
#include "sj_pwm.h"

SJ_PRIVATE enum v7_err PWM_set(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int ires = 0;
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t periodv = v7_arg(v7, 1);
  v7_val_t dutyv = v7_arg(v7, 2);
  int pin, period, duty;

  if (!v7_is_number(pinv) || !v7_is_number(periodv) || !v7_is_number(dutyv)) {
    rcode = v7_throwf(v7, "Error", "Numeric argument expected");
    goto clean;
  }

  pin = v7_to_number(pinv);
  period = v7_to_number(periodv);
  duty = v7_to_number(dutyv);

  ires = sj_pwm_set(pin, period, duty);

  *res = v7_mk_boolean(ires);
  goto clean;

clean:
  return rcode;
}

void sj_pwm_api_setup(struct v7 *v7) {
  v7_val_t pwm = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "PWM", ~0, pwm);
  v7_set_method(v7, pwm, "set", PWM_set);
}
