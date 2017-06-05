/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <OneWire.h>

OneWire *ow = NULL;

void setup(void) {
  printf("Arduino OnWire\r\n");
  ow = new OneWire(13);
}

void error(const char *s) {
  printf("E: %s\n", s);
  delay(1000);
}

void loop(void) {
  // This function reads data from the DS18B20 temperature sensor every second
  // Datasheet: http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
  byte rom[8], data[12];
  int16_t raw;
  byte cfg;

  ow->reset_search();
  if (!ow->search(rom)) {
    ow->reset_search();
    error("No device found");
    return;
  }

  if (rom[0] != 0x28) {
    error("Unknown device");
    return;
  }

  ow->reset();
  ow->select(rom);
  ow->write(0x44);

  delay(750);

  ow->reset();
  ow->select(rom);
  ow->write(0xBE);

  for (int i = 0; i < 9; i++) {
    data[i] = ow->read();
  }

  raw = (data[1] << 8) | data[0];
  cfg = (data[4] & 0x60);

  if (cfg == 0x00)
    raw = raw & ~7;
  else if (cfg == 0x20)
    raw = raw & ~3;
  else if (cfg == 0x40)
    raw = raw & ~1;

  printf("T:%f\n", (float) raw / 16.0);

  delay(350);
}
