#include <stdint.h>

#ifndef RTOS_SDK
#include "user_interface.h"
#endif

#include "v7_esp_hw.h"

uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;

  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}
