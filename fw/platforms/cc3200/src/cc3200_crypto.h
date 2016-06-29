/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Crypto primitives that use the CC3200 hardware engines.
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_CRYPTO_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_CRYPTO_H_

#include <inttypes.h>

enum cc3200_hash_algo {
  CC3200_HASH_ALGO_MD5 = 0,
  CC3200_HASH_ALGO_SHA1 = 2,
  CC3200_HASH_ALGO_SHA224 = 4,
  CC3200_HASH_ALGO_SHA256 = 6,
};

#define CC3200_HASH_BLOCK_SIZE 64

struct cc3200_hash_ctx {
  uint8_t block[CC3200_HASH_BLOCK_SIZE];
  unsigned int algo : 3;
  unsigned int inited : 1;
  unsigned int block_len : 8;
  unsigned int digest_len : 4;
  uint32_t digest[8];
};

void cc3200_hash_init(struct cc3200_hash_ctx *ctx, enum cc3200_hash_algo algo);
void cc3200_hash_update(struct cc3200_hash_ctx *ctx, const uint8_t *data,
                        uint32_t len);
void cc3200_hash_final(struct cc3200_hash_ctx *ctx, uint8_t *digest);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_CRYPTO_H_ */
