#include <ets_sys.h>

#include "esp_adc.h"
#include "esp_missing_includes.h"
#include "esp_periph.h"

#ifndef RTOS_SDK

#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>

#else

#include <stdlib.h>
#include <eagle_soc.h>
#include <pin_mux_register.h>
#include <adc_register.h>
#include <freertos/portmacro.h>
#include <disp_task.h>

#endif /* RTOS_SDK */

#ifndef ESP_ADC_DIVIDER
/*
 * ESP12 has an internal voltage divider to map 3.3V
 * to internal 1V on the A0 pin.
 *
 * If you want to Smart.js to return actual voltage
 * on an ESP12 board, please use the RSV (reserved) pin
 * right next to the A0 pin instead, or build your firmware with
 * -DESP_ADC_DIVIDER=3.3
 */
#define ESP_ADC_DIVIDER 1.0
#endif

uint32_t sj_adc_read(int pin) {
  return 0xFFFF & system_adc_read();
}

double sj_adc_read_voltage(int pin) {
  return sj_adc_read(pin) * ESP_ADC_DIVIDER / 1024.0;
}
