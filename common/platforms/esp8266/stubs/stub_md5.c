/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 *
 *
 * Computes MD5 digest of the specified region.
 *
 * Params:
 *   0 - Starting address
 *   1 - Length
 *   2 - Whether to output block checksums (every 4K) or not.
 *
 * Output:
 *   Each packet of length 16 will contain a digest. There will be variable
 *   number of these, depending on block checksum setting: if it's disabled,
 *   then only one will be sent, for entire region. If block checksums are
 *   enabled, then a digest for every 4K block is sent (including possibly
 *   incomplete last one). Full digest is still computed and sent regardless.
 *   So with block checksums one should expect ceil(size / 4096) + 1 digests.
 *   If the packet's length is 1, it's the final status: 0 - error, 1 - ok.
 *   0 status can be received at any point if flash read error occurs, 1 is
 *   sent at the very end.
 */

#include <inttypes.h>
#include "rom_functions.h"

uint32_t params[3] __attribute__((section(".params")));

/* #define HEX */

#ifdef HEX
uint8_t hex_digit(uint8_t n) {
  n &= 0xf;
  return n < 10 ? '0' + n : 'a' + n - 10;
}

void hex_str(const uint8_t *buf, uint32_t size, uint8_t *s) {
  for (int i = 0; i < size; i++) {
    *s++ = hex_digit(buf[i] >> 4);
    *s++ = hex_digit(buf[i]);
  }
}
#endif

void send_resp(uint8_t result) {
  send_packet(&result, 1);
  while (1) {
  }
}

void send_digest(const uint8_t *digest) {
#ifdef HEX
  uint8_t hex_digest[32];
  hex_str(digest, 16, hex_digest);
  send_packet(hex_digest, 32);
#else
  send_packet(digest, 16);
#endif
  uint8_t cr = '\n';
  send_packet(&cr, 1);
}

void stub_main() {
  uint8_t buf[4096];
  uint8_t digest[16];
  uint32_t addr = params[0];
  uint32_t len = params[1];
  uint32_t block_digest = params[2];
  struct MD5Context ctx;
  MD5Init(&ctx);
  while (len > 0) {
    uint32_t n = len;
    struct MD5Context block_ctx;
    MD5Init(&block_ctx);
    if (n > sizeof(buf)) n = sizeof(buf);
    if (SPIRead(addr, buf, n) != 0) send_resp(0);
    MD5Update(&ctx, buf, n);
    if (block_digest) {
      MD5Update(&block_ctx, buf, n);
      MD5Final(digest, &block_ctx);
      send_digest(digest);
    }
    addr += n;
    len -= n;
  }
  MD5Final(digest, &ctx);
  send_digest(digest);
  send_resp(1);
}
