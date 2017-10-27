/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cs_varint.h"

/*
 * Strings in AST are encoded as tuples (length, string).
 * Length is variable-length: if high bit is set in a byte, next byte is used.
 * Small string length (less then 128 bytes) is encoded in 1 byte.
 */
uint64_t cs_varint_decode(const uint8_t *p, int *llen) {
  unsigned int i = 0;
  unsigned int shift = 0;
  uint64_t num = 0;

  do {
    /*
     * Each byte of varint contains 7 bits, in little endian order.
     * MSB is a continuation bit: it tells whether next byte is used.
     */
    num |= ((uint64_t)(p[i] & 0x7f)) << shift;
    /*
     * First we increment i, then check whether it is within boundary and
     * whether decoded byte had continuation bit set.
     */
    i++;
    shift += 7;
  } while (shift < sizeof(uint64_t) * 8 && (p[i - 1] & 0x80));
  *llen = i;

  return num;
}

/* Return number of bytes to store length */
int cs_varint_llen(uint64_t num) {
  int n = 0;

  do {
    n++;
  } while (num >>= 7);

  return n;
}

int cs_varint_encode(uint64_t num, uint8_t *p) {
  int i, llen = cs_varint_llen(num);

  for (i = 0; i < llen; i++) {
    p[i] = (num & 0x7f) | (i < llen - 1 ? 0x80 : 0);
    num >>= 7;
  }

  return llen;
}
