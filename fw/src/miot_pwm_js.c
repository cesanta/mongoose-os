/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_JS && MIOT_ENABLE_PWM_API

#include "fw/src/miot_common.h"
#include "fw/src/miot_pwm.h"
#include "v7/v7.h"

MG_PRIVATE enum v7_err PWM_set(struct v7 *v7, v7_val_t *res) {
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

  pin = v7_get_double(v7, pinv);
  period = v7_get_double(v7, periodv);
  duty = v7_get_double(v7, dutyv);

  ires = miot_pwm_set(pin, period, duty);

  *res = v7_mk_boolean(v7, ires);
  goto clean;

clean:
  return rcode;
}

void miot_pwm_api_setup(struct v7 *v7) {
  v7_val_t pwm = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "PWM", ~0, pwm);
  v7_set_method(v7, pwm, "set", PWM_set);
}

#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_PWM_API */
