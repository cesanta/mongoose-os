/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include "common/platform.h"
#include "driver/adc.h"
#include "mgos_adc.h"

static int esp32_get_channel(int pin) {
  static int pins[] = {36, 37, 38, 39, 32, 33, 34, 35};
  static adc1_channel_t channels[] = {
      ADC1_CHANNEL_0, ADC1_CHANNEL_1, ADC1_CHANNEL_2, ADC1_CHANNEL_3,
      ADC1_CHANNEL_4, ADC1_CHANNEL_5, ADC1_CHANNEL_6, ADC1_CHANNEL_7};

  for (int i = 0; i < ARRAY_SIZE(pins); i++) {
    if (pins[i] == pin) return channels[i];
  }

  return -1;
}

bool mgos_adc_enable(int pin) {
  int ch;

  if ((ch = esp32_get_channel(pin)) == -1) return false;

  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ch, ADC_ATTEN_11db);

  return true;
}

int mgos_adc_read(int pin) {
  return adc1_get_voltage(esp32_get_channel(pin));
}
