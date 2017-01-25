/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <stdint.h>

#include "mbedtls/sha256.h"

/* For CryptoAuthLib host crypto. We use mbedTLS functions. */
int atcac_sw_sha2_256(const uint8_t *data, size_t data_size,
                      uint8_t digest[32]) {
  mbedtls_sha256(data, data_size, digest, false /* is_224 */);
  return 0;
}
