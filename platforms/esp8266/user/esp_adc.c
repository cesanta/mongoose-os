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

double sj_adc_read(int pin) {
  return (0xFFFF & system_adc_read()) / 1024.0;
}
