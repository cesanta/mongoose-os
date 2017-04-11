/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <stdint.h>

#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"

#include "mongoose/mongoose.h"

void mg_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  mbedtls_md5_context md5_ctx;
  mbedtls_md5_init(&md5_ctx);
  mbedtls_md5_starts(&md5_ctx);
  for (i = 0; i < num_msgs; i++) {
    mbedtls_md5_update(&md5_ctx, msgs[i], msg_lens[i]);
  }
  mbedtls_md5_finish(&md5_ctx, digest);
  mbedtls_md5_free(&md5_ctx);
}

void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  mbedtls_sha1_context sha1_ctx;
  mbedtls_sha1_init(&sha1_ctx);
  mbedtls_sha1_starts(&sha1_ctx);
  for (i = 0; i < num_msgs; i++) {
    mbedtls_sha1_update(&sha1_ctx, msgs[i], msg_lens[i]);
  }
  mbedtls_sha1_finish(&sha1_ctx, digest);
  mbedtls_sha1_free(&sha1_ctx);
}

/* For CryptoAuthLib host crypto. We use mbedTLS functions. */
int atcac_sw_sha2_256(const uint8_t *data, size_t data_size,
                      uint8_t digest[32]) {
  mbedtls_sha256(data, data_size, digest, false /* is_224 */);
  return 0;
}
