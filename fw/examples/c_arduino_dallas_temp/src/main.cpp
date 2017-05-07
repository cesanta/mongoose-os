/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire *ow;
DallasTemperature *dt;

void setup(void) {
  printf("Arduino Dallas Temperature sensors library example\n");

  // 1-Wire bus is plugged into GPIO13
  ow = new OneWire(13);
  dt = new DallasTemperature(ow);
  dt->begin();
}

void loop(void) {
  dt->requestTemperatures();
  printf("Temperature: %f *C\n", dt->getTempCByIndex(0));
}
