/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cs_varint.h"

size_t cs_varint_llen(uint64_t num) {
  size_t llen = 0;

  do {
    llen++;
  } while (num >>= 7);

  return llen;
}

size_t cs_varint_encode(uint64_t num, uint8_t *buf, size_t buf_size) {
  size_t llen = 0;

  do {
    uint8_t byte = num & 0x7f;
    num >>= 7;
    if (num != 0) byte |= 0x80;
    if (llen < buf_size) *buf++ = byte;
    llen++;
  } while (num != 0);

  return llen;
}

bool cs_varint_decode(const uint8_t *buf, size_t buf_size, uint64_t *num,
                      size_t *llen) {
  size_t i = 0, shift = 0;
  *num = 0;

  do {
    if (i == buf_size || i == (8 * sizeof(*num) / 7 + 1)) return false;
    /*
     * Each byte of varint contains 7 bits, in little endian order.
     * MSB is a continuation bit: it tells whether next byte is used.
     */
    *num |= ((uint64_t)(buf[i] & 0x7f)) << shift;
    /*
     * First we increment i, then check whether it is within boundary and
     * whether decoded byte had continuation bit set.
     */
    *llen = ++i;
    shift += 7;
  } while (shift < sizeof(uint64_t) * 8 && (buf[i - 1] & 0x80));

  return true;
}

uint64_t cs_varint_decode_unsafe(const uint8_t *buf, int *llen) {
  uint64_t v;
  size_t l;
  cs_varint_decode(buf, ~0, &v, &l);
  *llen = l;
  return v;
}
