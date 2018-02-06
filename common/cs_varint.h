/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_VARINT_H_
#define CS_COMMON_CS_VARINT_H_

#if defined(_WIN32) && _MSC_VER < 1700
typedef unsigned char uint8_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Returns number of bytes required to encode `num`. */
size_t cs_varint_llen(uint64_t num);

/*
 * Encodes `num` into `buf`.
 * Returns number of bytes required to encode `num`.
 * Note: return value may be greater than `buf_size` but the function will only
 * write `buf_size` bytes.
 */
size_t cs_varint_encode(uint64_t num, uint8_t *buf, size_t buf_size);

/*
 * Decodes varint stored in `buf`.
 * Stores the number of bytes consumed into `llen`.
 * If there aren't enough bytes in `buf` to decode a number, returns false.
 */
bool cs_varint_decode(const uint8_t *buf, size_t buf_size, uint64_t *num,
                      size_t *llen);

uint64_t cs_varint_decode_unsafe(const uint8_t *buf, int *llen);

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_CS_VARINT_H_ */
