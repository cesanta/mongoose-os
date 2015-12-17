/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 *
 *
 * Detects flash size within given range by doing binary search probing.
 * Assumes that flash size is a power of 2.
 *
 * Params: none.
 * Output: single packet with flash size or 0 if something went wrong.
 */

#include <inttypes.h>
#include "rom_functions.h"

void stub_main() {
  uint8_t dummy;
  uint32_t flash_size = 0;

  while (1) {
    if (SPIRead(flash_size, &dummy, 1) != 0) break;
    if (flash_size == 0) {
      /* Start at 256K */
      flash_size = 256 * 1024;
    } else {
      flash_size *= 2;
    }
  }

  send_packet(&flash_size, sizeof(flash_size));

  while (1) {
  }
}
