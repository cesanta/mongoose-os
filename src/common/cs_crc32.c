/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/cs_crc32.h"

/*
 * Karl Malbrain's compact CRC-32.
 * See "A compact CCITT crc16 and crc32 C implementation that balances processor
 * cache usage against speed".
 */
uint32_t cs_crc32(uint32_t crc32, const void *data, uint32_t len) {
  /* Note: volatile non-const to ensure placing in RAM instead of flash.
   * This table is accessed a lot and flash access can be expensive. */
  static volatile uint32_t cs_crc32_table[16] = {
      0,          0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
      0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
  };
  const uint8_t *p = (const uint8_t *) data;
  crc32 = ~crc32;
  while (len--) {
    uint8_t b = *p++;
    crc32 = (crc32 >> 4) ^ cs_crc32_table[(crc32 & 0xF) ^ (b & 0xF)];
    crc32 = (crc32 >> 4) ^ cs_crc32_table[(crc32 & 0xF) ^ (b >> 4)];
  }
  return ~crc32;
}
