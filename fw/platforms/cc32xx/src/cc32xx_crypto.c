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

#include "cc32xx_crypto.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_shamd5.h"

#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/shamd5.h"

#include "common/cs_dbg.h"

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
  }
  HWREG(SHAMD5_BASE + SHAMD5_O_MODE) = mode;
  while (!(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_CONTEXT_READY))
    ;
}

void cc32xx_hash_init(struct cc32xx_hash_ctx *ctx, enum cc32xx_hash_algo algo) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->algo = algo;
}

void cc32xx_hash_update(struct cc32xx_hash_ctx *ctx, const uint8_t *data,
                        uint32_t len) {
  if (ctx->block_len > 0) {
    uint32_t block_remain = CC32XX_HASH_BLOCK_SIZE - ctx->block_len;
    if (block_remain > len) block_remain = len;
    memcpy(ctx->block + ctx->block_len, data, block_remain);
    ctx->block_len += block_remain;
    data += block_remain;
    len -= block_remain;
    if (ctx->block_len < CC32XX_HASH_BLOCK_SIZE) return;
  }
  const uint32_t to_hash =
      (((uint32_t) ctx->block_len) + len) & ~(CC32XX_HASH_BLOCK_SIZE - 1);
  if (to_hash > 0) {
    vPortEnterCritical();
    init_engine(ctx, 0);
    HWREG(SHAMD5_BASE + SHAMD5_O_LENGTH) = to_hash;
    if (ctx->block_len == CC32XX_HASH_BLOCK_SIZE) {
      while (
          !(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_INPUT_READY))
        ;
      uint32_t *p = (uint32_t *) ctx->block;
      for (int i = 0; i < CC32XX_HASH_BLOCK_SIZE; i += 4) {
        HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p++;
      }
    }
    while (len >= CC32XX_HASH_BLOCK_SIZE) {
      while (
          !(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_INPUT_READY))
        ;
      for (int i = 0; i < CC32XX_HASH_BLOCK_SIZE; i += 4) {
        HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *((uint32_t *) data);
        data += 4;
        len -= 4;
      }
    }
    while (!(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_OUTPUT_READY))
      ;
    MAP_SHAMD5ResultRead(SHAMD5_BASE, (uint8_t *) ctx->digest);
    /* Must read count register to finish the round. */
    HWREG(SHAMD5_BASE + SHAMD5_O_DIGEST_COUNT);
    MAP_PRCMPeripheralClkDisable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
    vPortExitCritical();
  }
  memcpy(ctx->block, data, len);
  ctx->block_len = len;
}

void cc32xx_hash_final(struct cc32xx_hash_ctx *ctx, uint8_t *digest) {
  vPortEnterCritical();
  init_engine(ctx, SHAMD5_MODE_CLOSE_HASH);
  HWREG(SHAMD5_BASE + SHAMD5_O_LENGTH) = ctx->block_len;
  int bl = ctx->block_len;
  int i = 0;
  uint32_t *p = (uint32_t *) ctx->block;
  while (bl - i > 4) {
    HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p++;
    i += 4;
  }
  if (i < bl) {
    for (int j = bl; j % 4 != 0; j++) ctx->block[j] = 0;
    HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p;
  }
  while (!(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_OUTPUT_READY))
    ;
  MAP_SHAMD5ResultRead(SHAMD5_BASE, digest);
  /* Must read count register to finish the round. */
  HWREG(SHAMD5_BASE + SHAMD5_O_DIGEST_COUNT);
  MAP_PRCMPeripheralClkDisable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
  vPortExitCritical();
}

/* Mongoose external hash interface. */
void mg_hash_md5_v(uint32_t num_msgs, const uint8_t *msgs[],
                   const uint32_t *msg_lens, uint8_t *digest) {
  struct cc32xx_hash_ctx ctx;
  cc32xx_hash_init(&ctx, CC32XX_HASH_ALGO_MD5);
  for (int i = 0; i < num_msgs; i++) {
    cc32xx_hash_update(&ctx, msgs[i], msg_lens[i]);
  }
  cc32xx_hash_final(&ctx, digest);
}

void mg_hash_sha1_v(uint32_t num_msgs, const uint8_t *msgs[],
                    const uint32_t *msg_lens, uint8_t *digest) {
  struct cc32xx_hash_ctx ctx;
  cc32xx_hash_init(&ctx, CC32XX_HASH_ALGO_SHA1);
  for (int i = 0; i < num_msgs; i++) {
    cc32xx_hash_update(&ctx, msgs[i], msg_lens[i]);
  }
  cc32xx_hash_final(&ctx, digest);
}
