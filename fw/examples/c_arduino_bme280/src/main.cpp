/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <Adafruit_BME280.h>

#define SENSOR_ADDR 0x76

static Adafruit_BME280 *s_bme = nullptr;

void setup(void) {
  printf("Arduino Adafruit_BME280 library simple example\n");

  // Initialize Adafruit_BME280 library
  s_bme = new Adafruit_BME280();

  // Initialize sensor
  if (!s_bme->begin(SENSOR_ADDR)) {
    printf("Can't find a sensor\n");
    while (true) delay(1000);
  }
}

void loop(void) {
  delay(2000);
  printf("Temperature: %.2f *C\n", s_bme->readTemperature());
  printf("Humidity: %.2f %%RH\n", s_bme->readHumidity());
  printf("Pressure: %.2f kPa\n\n", s_bme->readPressure() / 1000.0);
}
