/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONEWIRE_PIN 13  // 1-Wire bus is plugged into GPIO13

static OneWire *s_ow = nullptr;
static DallasTemperature *s_dt = nullptr;
static int s_n = 0;       // Number of sensors found on the 1-Wire bus
static uint8_t **s_sens;  // Sensors addresses

void setup(void) {
  printf("Arduino DallasTemperature library simple example\n");

  // Initialize a OneWire handle
  s_ow = new OneWire(ONEWIRE_PIN);
  // Pass a OneWire handle to Dallas Temperature
  s_dt = new DallasTemperature(s_ow);
  // Start up the library
  s_dt->begin();

  if ((s_n = s_dt->getDeviceCount()) == 0) {
    printf("No sensors found\n");
    while (true) delay(1000);
  } else {
    printf("Num of sensors found: %d\n", s_n);
  }

  s_sens = new uint8_t *[s_n];

  for (int i = 0; i < s_n; i++) {
    s_sens[i] = new uint8_t[8];
    if (s_dt->getAddress(s_sens[i], i)) {
      printf("Sens#%d address: ", i + 1);
      for (int j = 0; j < 8; j++) {
        printf("%x", s_sens[i][j]);
      }
      printf("\n");
    }
  }
}

void loop(void) {
  s_dt->requestTemperatures();
  for (int i = 0; i < s_n; i++) {
    printf("Sens#%d temperature: %f *C\n", i + 1, s_dt->getTempC(s_sens[i]));
  }
  delay(1000);
}
