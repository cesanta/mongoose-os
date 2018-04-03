/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_CRYPTO_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_CRYPTO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cc32xx_hash_algo {
  CC32XX_HASH_ALGO_MD5 = 0,
  CC32XX_HASH_ALGO_SHA1 = 2,
  CC32XX_HASH_ALGO_SHA224 = 4,
  CC32XX_HASH_ALGO_SHA256 = 6,
};

#define CC32XX_HASH_BLOCK_SIZE 64

struct cc32xx_hash_ctx {
  uint8_t block[CC32XX_HASH_BLOCK_SIZE];
  unsigned int algo : 3;
  unsigned int inited : 1;
  unsigned int block_len : 8;
  unsigned int digest_len : 4;
  uint32_t digest[8];
};

void cc32xx_hash_init(struct cc32xx_hash_ctx *ctx, enum cc32xx_hash_algo algo);
void cc32xx_hash_update(struct cc32xx_hash_ctx *ctx, const uint8_t *data,
                        uint32_t len);
void cc32xx_hash_final(struct cc32xx_hash_ctx *ctx, uint8_t *digest);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_CRYPTO_H_ */
