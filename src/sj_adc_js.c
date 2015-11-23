#include <v7.h>
#include "sj_adc.h"

#ifndef SJ_DISABLE_ADC

static v7_val_t ADC_read(struct v7 *v7) {
  v7_val_t pinv = v7_arg(v7, 0);
  int pin;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    return v7_create_undefined();
  }

  pin = v7_to_number(pinv);
  return v7_create_number(sj_adc_read(pin));
}

void init_adcjs(struct v7 *v7) {
  v7_val_t adc = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "ADC", 3, 0, adc);
  v7_set_method(v7, adc, "read", ADC_read);
}

#else

void init_gpiojs(struct v7 *v7) {
  (void) v7;
}

#endif /* SJ_DISABLE_ADC */
