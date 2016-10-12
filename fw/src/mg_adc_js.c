/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_adc.h"
#include "fw/src/mg_common.h"
#include "v7/v7.h"

#ifdef MG_ENABLE_ADC_API

MG_PRIVATE enum v7_err ADC_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = V7_UNDEFINED;
  } else {
    pin = v7_get_double(v7, pinv);
    *res = v7_mk_number(v7, mg_adc_read(pin));
  }

  return V7_OK;
}

MG_PRIVATE enum v7_err ADC_readVoltage(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = V7_UNDEFINED;
  } else {
    pin = v7_get_double(v7, pinv);
    *res = v7_mk_number(v7, mg_adc_read_voltage(pin));
  }

  return V7_OK;
}

void mg_adc_api_setup(struct v7 *v7) {
  v7_val_t adc = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "ADC", ~0, adc);
  v7_set_method(v7, adc, "read", ADC_read);
  v7_set_method(v7, adc, "readVoltage", ADC_readVoltage);
}

#endif /* MG_ENABLE_ADC_API */
