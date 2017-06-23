/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * See on GitHub:
 * [mgos_adc.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_adc.h),
 * [mgos_adc.c](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_adc.c)
 */

#ifndef CS_FW_SRC_MGOS_ADC_H_
#define CS_FW_SRC_MGOS_ADC_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Configure and enable ADC */
bool mgos_adc_enable(int pin);

/* Read from the analog pin */
int mgos_adc_read(int pin);

/* Return the voltage read by the ADC */
double mgos_adc_read_voltage(int pin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_ADC_H_ */
