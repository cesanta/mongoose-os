/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_VARINT_H_
#define CS_COMMON_CS_VARINT_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

int cs_varint_encode(int64_t num, uint8_t *to);
int64_t cs_varint_decode(const uint8_t *from, int *llen);
int cs_varint_llen(int64_t num);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_VARINT_H_ */
