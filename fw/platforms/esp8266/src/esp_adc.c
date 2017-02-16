/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_adc.h"

#ifdef RTOS_SDK
#include <esp_system.h>
#else
#include <user_interface.h>
#endif

uint32_t mgos_adc_read(int pin) {
  (void) pin;
  return 0xFFFF & system_adc_read();
}

double mgos_adc_read_voltage(int pin) {
  return mgos_adc_read(pin) / 1024.0; /* ESP8266 has a 10-bit ADC */
}
