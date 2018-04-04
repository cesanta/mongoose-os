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

#include "cc32xx_hash.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_shamd5.h"

#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/shamd5.h"

#include "mgos_system.h"

/* mbedTLS interface */
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"

static struct mgos_rlock_type *s_engine_lock = NULL;

static inline void wait_ready(uint32_t flag) {
  while (!(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & flag)) {
  }
}

static void init_engine(struct cc32xx_hash_ctx *ctx, uint32_t mode) {
  MAP_PRCMPeripheralClkEnable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
  mode |= ctx->algo;
  if (!ctx->inited) {
    mode |= SHAMD5_MODE_ALGO_CONSTANT;
    ctx->inited = 1;
  } else {
    for (int i = 0; i < 8; i++) {
      HWREG(SHAMD5_BASE + SHAMD5_O_IDIGEST_A + i * 4) = ctx->digest[i];
    }
    HWREG(SHAMD5_BASE + SHAMD5_O_DIGEST_COUNT) = ctx->digest_count;
  }
  HWREG(SHAMD5_BASE + SHAMD5_O_MODE) = mode;
  wait_ready(SHAMD5_INT_CONTEXT_READY);
}

void cc32xx_hash_init(void *vctx) {
  memset(vctx, 0, sizeof(struct cc32xx_hash_ctx));
}

void cc32xx_hash_free(void *vctx) {
  if (vctx == NULL) return;
  cc32xx_hash_init(vctx);
}

void cc32xx_hash_clone(void *dst, void *src) {
  memcpy(dst, src, sizeof(struct cc32xx_hash_ctx));
}

int cc32xx_hash_start(void *vctx, enum cc32xx_hash_algo algo) {
  struct cc32xx_hash_ctx *ctx = (struct cc32xx_hash_ctx *) vctx;
  cc32xx_hash_init(vctx);
  ctx->algo = algo;
  return 0;
}

int cc32xx_hash_update(void *vctx, const unsigned char *data, size_t len) {
  struct cc32xx_hash_ctx *ctx = (struct cc32xx_hash_ctx *) vctx;
  if (ctx->block_len > 0) {
    uint32_t block_remain = CC32XX_HASH_BLOCK_SIZE - ctx->block_len;
    if (block_remain > len) block_remain = len;
    memcpy(ctx->block + ctx->block_len, data, block_remain);
    ctx->block_len += block_remain;
    data += block_remain;
    len -= block_remain;
    if (ctx->block_len < CC32XX_HASH_BLOCK_SIZE) return 0;
  }
  const uint32_t to_hash =
      (((uint32_t) ctx->block_len) + len) & ~(CC32XX_HASH_BLOCK_SIZE - 1);
  if (to_hash > 0) {
    mgos_rlock(s_engine_lock);
    init_engine(ctx, 0);
    HWREG(SHAMD5_BASE + SHAMD5_O_LENGTH) = to_hash;
    if (ctx->block_len == CC32XX_HASH_BLOCK_SIZE) {
      wait_ready(SHAMD5_INT_INPUT_READY);
      uint32_t *p = (uint32_t *) ctx->block;
      for (int i = 0; i < CC32XX_HASH_BLOCK_SIZE; i += 4) {
        HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p++;
      }
    }
    while (len >= CC32XX_HASH_BLOCK_SIZE) {
      wait_ready(SHAMD5_INT_INPUT_READY);
      for (int i = 0; i < CC32XX_HASH_BLOCK_SIZE; i += 4) {
        HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *((uint32_t *) data);
        data += 4;
        len -= 4;
      }
    }
    wait_ready(SHAMD5_INT_OUTPUT_READY);
    MAP_SHAMD5ResultRead(SHAMD5_BASE, (uint8_t *) ctx->digest);
    ctx->digest_count = HWREG(SHAMD5_BASE + SHAMD5_O_DIGEST_COUNT);
    mgos_runlock(s_engine_lock);
  }
  memcpy(ctx->block, data, len);
  ctx->block_len = len;
  return 0;
}

int cc32xx_hash_finish(void *vctx, unsigned char *digest) {
  struct cc32xx_hash_ctx *ctx = (struct cc32xx_hash_ctx *) vctx;
  mgos_rlock(s_engine_lock);
  init_engine(ctx, SHAMD5_MODE_CLOSE_HASH);
  HWREG(SHAMD5_BASE + SHAMD5_O_LENGTH) = ctx->block_len;
  if (ctx->block_len > 0) {
    wait_ready(SHAMD5_INT_INPUT_READY);
    uint32_t *p = (uint32_t *) ctx->block;
    /* Note: writing up to 3 garbage bytes at the tail is fine. */
    for (int i = 0; i < CC32XX_HASH_BLOCK_SIZE && i < ctx->block_len; i += 4) {
      HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p++;
    }
  }
  wait_ready(SHAMD5_INT_OUTPUT_READY);
  MAP_SHAMD5ResultRead(SHAMD5_BASE, digest);
  /* Must read count register to finish the round. */
  ctx->digest_count = HWREG(SHAMD5_BASE + SHAMD5_O_DIGEST_COUNT);
  MAP_PRCMPeripheralClkDisable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
  mgos_runlock(s_engine_lock);
  ctx->block_len = 0;
  return 0;
}

int cc32xx_hash_internal_process(void *ctx, const unsigned char data[64]) {
  cc32xx_hash_update(ctx, data, 64);
  return 0;
}

#ifdef MBEDTLS_MD5_ALT
void mbedtls_md5_init(mbedtls_md5_context *ctx)
    __attribute__((alias("cc32xx_hash_init")));
void mbedtls_md5_clone(mbedtls_md5_context *dst, const mbedtls_md5_context *src)
    __attribute__((alias("cc32xx_hash_clone")));
int mbedtls_md5_starts_ret(mbedtls_md5_context *ctx) {
  cc32xx_hash_start(ctx, CC32XX_HASH_ALGO_MD5);
  return 0;
}
int mbedtls_md5_update_ret(mbedtls_md5_context *ctx, const unsigned char *input,
                           size_t ilen)
    __attribute__((alias("cc32xx_hash_update")));
int mbedtls_md5_finish_ret(mbedtls_md5_context *ctx, unsigned char output[32])
    __attribute__((alias("cc32xx_hash_finish")));
void mbedtls_md5_free(mbedtls_md5_context *ctx)
    __attribute__((alias("cc32xx_hash_free")));
int mbedtls_internal_md5_process(mbedtls_md5_context *ctx,
                                 const unsigned char data[64])
    __attribute__((alias("cc32xx_hash_internal_process")));
#endif /* MBEDTLS_MD5_ALT */

#ifdef MBEDTLS_SHA1_ALT
void mbedtls_sha1_init(mbedtls_sha1_context *ctx)
    __attribute__((alias("cc32xx_hash_init")));
void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
                        const mbedtls_sha1_context *src)
    __attribute__((alias("cc32xx_hash_clone")));
int mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx) {
  cc32xx_hash_start(ctx, CC32XX_HASH_ALGO_SHA1);
  return 0;
}
int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx,
                            const unsigned char *input, size_t ilen)
    __attribute__((alias("cc32xx_hash_update")));
int mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx, unsigned char output[32])
    __attribute__((alias("cc32xx_hash_finish")));
void mbedtls_sha1_free(mbedtls_sha1_context *ctx)
    __attribute__((alias("cc32xx_hash_free")));
int mbedtls_internal_sha1_process(mbedtls_sha1_context *ctx,
                                  const unsigned char data[64])
    __attribute__((alias("cc32xx_hash_internal_process")));
#endif /* MBEDTLS_SHA1_ALT */

#ifdef MBEDTLS_SHA256_ALT
void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
    __attribute__((alias("cc32xx_hash_init")));
void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                          const mbedtls_sha256_context *src)
    __attribute__((alias("cc32xx_hash_clone")));
int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224) {
  cc32xx_hash_start(
      ctx, (is224 ? CC32XX_HASH_ALGO_SHA224 : CC32XX_HASH_ALGO_SHA256));
  return 0;
}
int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
                              const unsigned char *input, size_t ilen)
    __attribute__((alias("cc32xx_hash_update")));
int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx,
                              unsigned char output[32])
    __attribute__((alias("cc32xx_hash_finish")));
void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
    __attribute__((alias("cc32xx_hash_free")));
int mbedtls_internal_sha256_process(mbedtls_sha256_context *ctx,
                                    const unsigned char data[64])
    __attribute__((alias("cc32xx_hash_internal_process")));
#endif /* MBEDTLS_SHA256_ALT */

/* Mongoose external hash interface. */
void mg_hash_md5_v(uint32_t num_msgs, const uint8_t *msgs[],
                   const uint32_t *msg_lens, uint8_t *digest) {
  struct cc32xx_hash_ctx ctx;
  cc32xx_hash_init(&ctx);
  cc32xx_hash_start(&ctx, CC32XX_HASH_ALGO_MD5);
  for (int i = 0; i < num_msgs; i++) {
    cc32xx_hash_update(&ctx, msgs[i], msg_lens[i]);
  }
  cc32xx_hash_finish(&ctx, digest);
}

void mg_hash_sha1_v(uint32_t num_msgs, const uint8_t *msgs[],
                    const uint32_t *msg_lens, uint8_t *digest) {
  struct cc32xx_hash_ctx ctx;
  cc32xx_hash_init(&ctx);
  cc32xx_hash_start(&ctx, CC32XX_HASH_ALGO_SHA1);
  for (int i = 0; i < num_msgs; i++) {
    cc32xx_hash_update(&ctx, msgs[i], msg_lens[i]);
  }
  cc32xx_hash_finish(&ctx, digest);
}

void cc32xx_hash_module_init(void) {
  s_engine_lock = mgos_rlock_create();
}
