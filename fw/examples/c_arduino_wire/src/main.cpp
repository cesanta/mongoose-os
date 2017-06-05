/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This Arduino like Wire library example shows how can use
 * the BOSCH BME280 combined humidity and pressure sensor.
 * Datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/
 *            datasheets/BST-BME280_DS001-11.pdf
 */

#include "bme280.h"

// Wire handle
TwoWire *w = NULL;

uint8_t read_byte(uint8_t addr, uint8_t reg) {
  w->beginTransmission(addr);
  w->write(reg);
  w->endTransmission();
  w->requestFrom(addr, (uint8_t) 1);
  return w->read();
}

uint16_t read_word(uint8_t addr, uint8_t reg) {
  w->beginTransmission(addr);
  w->write(reg);
  w->endTransmission();
  w->requestFrom(addr, (uint8_t) 2);
  return (w->read() << 8) | w->read();
}

uint16_t read_word_le(uint8_t addr, uint8_t reg) {
  // little-endian
  uint16_t data = read_word(addr, reg);
  return (data >> 8) | (data << 8);
}

uint32_t read(uint8_t addr, uint8_t reg) {
  uint32_t data;

  w->beginTransmission(addr);
  w->write(reg);
  w->endTransmission();
  w->requestFrom(addr, (uint8_t) 3);

  data = w->read();
  data <<= 8;
  data |= w->read();
  data <<= 8;
  data |= w->read();

  return data;
}

void write_byte(uint8_t addr, uint8_t reg, uint8_t value) {
  w->beginTransmission(addr);
  w->write(reg);
  w->write(value);
  w->endTransmission();
}

void error(const char *s) {
  printf("Error: %s\n", s);
  delay(1000);
}

void setup(void) {
  printf("Arduino Wire library example\n");

  // Initialize Wire library
  w = new TwoWire();
  w->begin();

  // Initialize BME280
  if (!init()) {
    error("Can't find a sensor\n");
    while (true) delay(1000);
  }
}

void loop(void) {
  delay(2000);
  printf("Temperature: %f *C\n", getTemp());
  printf("Humidity: %f %%RH\n", getHumi());
  printf("Pressure: %f kPa\n\n", getPress());
}
