/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>

#include "esp_adc.h"
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "esp_periph.h"

#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>

uint32_t miot_adc_read(int pin) {
  (void) pin;
  return 0xFFFF & system_adc_read();
}

double miot_adc_read_voltage(int pin) {
  return miot_adc_read(pin) / 1024.0; /* ESP8266 has a 10-bit ADC */
}
