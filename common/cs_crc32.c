/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_crc32.h"

/*
 * Karl Malbrain's compact CRC-32.
 * See "A compact CCITT crc16 and crc32 C implementation that balances processor
 * cache usage against speed".
 */
uint32_t cs_crc32(uint32_t crc32, const uint8_t *ptr, uint32_t buf_len) {
  static const uint32_t s_crc32[16] = {
      0,          0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
      0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  crc32 = ~crc32;
  while (buf_len--) {
    uint8_t b = *ptr++;
    crc32 = (crc32 >> 4) ^ s_crc32[(crc32 & 0xF) ^ (b & 0xF)];
    crc32 = (crc32 >> 4) ^ s_crc32[(crc32 & 0xF) ^ (b >> 4)];
  }
  return ~crc32;
}
