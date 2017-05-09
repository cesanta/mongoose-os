/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONEWIRE_PIN 13  // 1-Wire bus is plugged into GPIO13
#define NUM_SENS 2      // Number of sensors on the 1-Wire bus

OneWire *ow = NULL;
DallasTemperature *dt = NULL;
DeviceAddress sens[NUM_SENS];

void setup(void) {
  printf("Arduino Dallas Temperature library very simple example\n");

  // Initialize a OneWire handle
  ow = new OneWire(ONEWIRE_PIN);
  // Pass a OneWire handle to Dallas Temperature
  dt = new DallasTemperature(ow);
  // Start up the library
  dt->begin();

  // Search for devices on the 1-Wire bus
  ow->reset_search();
  for (int i = 0; i < NUM_SENS; i++) {
    if (!ow->search(sens[i])) {
      printf("Can't find sensor #%d\n", i + 1);
    } else {
      printf("Sens#%d address: ", i + 1);
      for (int j = 0; j < 8; j++) {
        printf("%x", sens[i][j]);
      }
      printf("\n");
    }
  }
}

void loop(void) {
  dt->requestTemperatures();
  delay(1000);
  for (int i = 0; i < NUM_SENS; i++) {
    printf("Sens#%d Temperature: %f *C\n", i + 1, dt->getTempC(sens[i]));
  }
}
