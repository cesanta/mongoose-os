/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_VARINT_H_
#define CS_COMMON_CS_VARINT_H_

#include <stdio.h>

#if defined(_WIN32) && _MSC_VER < 1700
typedef unsigned char uint8_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

int cs_varint_encode(uint64_t num, uint8_t *to);
uint64_t cs_varint_decode(const uint8_t *from, int *llen);
int cs_varint_llen(uint64_t num);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_VARINT_H_ */
