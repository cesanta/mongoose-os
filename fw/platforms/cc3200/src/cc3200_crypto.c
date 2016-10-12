/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/cc3200/src/cc3200_crypto.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_shamd5.h"
#include "rom.h"
#include "rom_map.h"

#include "driverlib/prcm.h"
#include "driverlib/shamd5.h"

#include "common/cs_dbg.h"

static void init_engine(struct cc3200_hash_ctx *ctx, uint32_t mode) {
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

void cc3200_hash_init(struct cc3200_hash_ctx *ctx, enum cc3200_hash_algo algo) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->algo = algo;
}

void cc3200_hash_update(struct cc3200_hash_ctx *ctx, const uint8_t *data,
                        uint32_t len) {
  if (ctx->block_len > 0) {
    uint32_t block_remain = CC3200_HASH_BLOCK_SIZE - ctx->block_len;
    if (block_remain > len) block_remain = len;
    memcpy(ctx->block + ctx->block_len, data, block_remain);
    ctx->block_len += block_remain;
    data += block_remain;
    len -= block_remain;
    if (ctx->block_len < CC3200_HASH_BLOCK_SIZE) return;
  }
  const uint32_t to_hash =
      (((uint32_t) ctx->block_len) + len) & ~(CC3200_HASH_BLOCK_SIZE - 1);
  if (to_hash > 0) {
    vPortEnterCritical();
    init_engine(ctx, 0);
    HWREG(SHAMD5_BASE + SHAMD5_O_LENGTH) = to_hash;
    if (ctx->block_len == CC3200_HASH_BLOCK_SIZE) {
      while (
          !(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_INPUT_READY))
        ;
      uint32_t *p = (uint32_t *) ctx->block;
      for (int i = 0; i < CC3200_HASH_BLOCK_SIZE; i += 4) {
        HWREG(SHAMD5_BASE + SHAMD5_O_DATA0_IN + i) = *p++;
      }
    }
    while (len >= CC3200_HASH_BLOCK_SIZE) {
      while (
          !(HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_INPUT_READY))
        ;
      for (int i = 0; i < CC3200_HASH_BLOCK_SIZE; i += 4) {
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

void cc3200_hash_final(struct cc3200_hash_ctx *ctx, uint8_t *digest) {
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
void mg_hash_sha1_v(uint32_t num_msgs, const uint8_t *msgs[],
                    const uint32_t *msg_lens, uint8_t *digest) {
  struct cc3200_hash_ctx ctx;
  cc3200_hash_init(&ctx, CC3200_HASH_ALGO_SHA1);
  for (int i = 0; i < num_msgs; i++) {
    cc3200_hash_update(&ctx, msgs[i], msg_lens[i]);
  }
  cc3200_hash_final(&ctx, digest);
}

/* Test code */
#if 0
void bin2hex(const uint8_t *src, int src_len, char *dst);

const char *data =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
    "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum";

void test_crypto(void) {
  uint8_t digest[32];
  char digest_str[65];
  const char *s1 = "The quick ", *s2 = "brown fox jumps over the lazy dog";
  const uint8_t *msgs[2] = {(const uint8_t *) s1, (const uint8_t *) s2};
  const size_t msg_lens[2] = {strlen(s1), strlen(s2)};
  /*
    const char *s = "The quick brown fox jumps over the lazy dog";
    const uint8_t *msgs[1] = {(const uint8_t *) s};
    const size_t msg_lens[1] = {strlen(s)};
  */
  mg_hash_sha1_v(2, msgs, msg_lens, digest);
  bin2hex(digest, 20, digest_str);
  LOG(LL_INFO, ("%s", digest_str));
  {
    struct cc3200_hash_ctx ctx0, ctx1, ctx2;
    cc3200_hash_init(&ctx0, CC3200_HASH_ALGO_SHA1);
    cc3200_hash_final(&ctx0, digest);
    bin2hex(digest, 20, digest_str);
    LOG(LL_INFO, ("d0 %s", digest_str));
    cc3200_hash_init(&ctx1, CC3200_HASH_ALGO_SHA1);
    cc3200_hash_init(&ctx2, CC3200_HASH_ALGO_SHA256);
    cc3200_hash_update(&ctx1, (const uint8_t *) data, 99);
    cc3200_hash_update(&ctx2, (const uint8_t *) data, 59);
    cc3200_hash_update(&ctx1, (const uint8_t *) data + 99, 100);
    cc3200_hash_update(&ctx1, (const uint8_t *) data + 99 + 100,
                       strlen(data) - 99 - 100);
    cc3200_hash_update(&ctx2, (const uint8_t *) data + 59, strlen(data) - 59);
    cc3200_hash_final(&ctx1, digest);
    bin2hex(digest, 20, digest_str);
    LOG(LL_INFO, ("d1 %s", digest_str));
    cc3200_hash_final(&ctx2, digest);
    bin2hex(digest, 32, digest_str);
    LOG(LL_INFO, ("d2 %s", digest_str));
  }
}
#endif
