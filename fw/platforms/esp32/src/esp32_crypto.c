/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>

#include "mbedtls/sha256.h"

/* For CryptoAuthLib host crypto. We use mbedTLS functions. */
int atcac_sw_sha2_256(const uint8_t *data, size_t data_size,
                      uint8_t digest[32]) {
  mbedtls_sha256_context ctx;

  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_update(&ctx, data, data_size);
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  return 0;
}
