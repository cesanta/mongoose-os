/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/v7.h"
#include "sj_adc.h"

#ifndef SJ_DISABLE_ADC

static enum v7_err ADC_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = v7_mk_undefined();
  } else {
    pin = v7_to_number(pinv);
    *res = v7_mk_number(sj_adc_read(pin));
  }

  return V7_OK;
}

static enum v7_err ADC_readVoltage(struct v7 *v7, v7_val_t *res) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    *res = v7_mk_undefined();
  } else {
    pin = v7_to_number(pinv);
    *res = v7_mk_number(sj_adc_read_voltage(pin));
  }

  return V7_OK;
}

void init_adcjs(struct v7 *v7) {
  v7_val_t adc = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "ADC", ~0, adc);
  v7_set_method(v7, adc, "read", ADC_read);
  v7_set_method(v7, adc, "readVoltage", ADC_readVoltage);
}

#else

void init_gpiojs(struct v7 *v7) {
  (void) v7;
}

#endif /* SJ_DISABLE_ADC */
