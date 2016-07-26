/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_COMMON_CS_CRC32_H_
#define CS_COMMON_CS_CRC32_H_

#include <inttypes.h>

uint32_t cs_crc32(uint32_t crc, const uint8_t *ptr, uint32_t buf_len);

#endif /* CS_COMMON_CS_CRC32_H_ */
