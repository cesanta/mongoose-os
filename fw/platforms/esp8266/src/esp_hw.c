/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdio.h>

#include "user_interface.h"

#include "esp_hw.h"
#include "esp_fs.h"

IRAM NOINSTR uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;
  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}
