/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/src/esp_hw.h"

IRAM NOINSTR uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;
  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}
