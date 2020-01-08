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

#include "common/cs_hex.h"

#include <ctype.h>

static int hextoi(int x) {
  return (x >= '0' && x <= '9' ? x - '0' : x - 'W');
}

int cs_hex_decode(const char *s, int len, unsigned char *dst, int *dst_len) {
  int i = 0;
  unsigned char *p = dst;
  while (i < len) {
    int c1, c2;
    c1 = hextoi(tolower((int) s[i++]));
    if (c1 < 0 || c1 > 15 || i == len) {
      i--;
      break;
    }
    c2 = hextoi(tolower((int) s[i++]));
    if (c2 < 0 || c2 > 15) {
      i -= 2;
      break;
    }
    *p++ = (unsigned char) ((c1 << 4) | c2);
  }
  *dst_len = (int) (p - dst);
  return i;
}
