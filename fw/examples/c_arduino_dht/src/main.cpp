/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "DHT.h"

DHT *dht = NULL;

void setup() {
  printf("Arduino DHT example\n");
  dht = new DHT(13, DHT22);
  dht->begin();
}

void loop() {
  delay(2000);

  float h = dht->readHumidity();
  float t = dht->readTemperature();

  if (isnan(h) || isnan(t)) {
    printf("Failed to read data from sensor\n");
    return;
  }

  printf("Temperature: %f *C Humidity: %f %% \n", t, h);
}
