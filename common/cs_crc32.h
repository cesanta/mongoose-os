/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_COMMON_CS_CRC32_H_
#define CS_COMMON_CS_CRC32_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t cs_crc32(uint32_t crc, const void *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_CS_CRC32_H_ */
