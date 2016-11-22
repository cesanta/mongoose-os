/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdlib.h>

#include "osapi.h"

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "mongoose/mongoose.h"

/* For WebSocket handshake. */
void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  (void) sha1_vector(num_msgs, msgs, msg_lens, digest);
}

#if MG_ENABLE_SSL

/* For Krypton */
#if MG_SSL_IF != MG_SSL_IF_MBEDTLS

#include "krypton/krypton.h"

#ifdef KR_EXT_MD5
void kr_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) {
  (void) md5_vector(num_msgs, msgs, msg_lens, digest);
}
#endif

#ifdef KR_EXT_SHA1
void kr_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  (void) sha1_vector(num_msgs, msgs, msg_lens, digest);
}
#endif

#ifdef KR_EXT_AES

#define AES_BLOCK_SIZE 16
#define AES128_KEY_SIZE 16
#define AES128_IV_SIZE 16

static void *esp_aes128_new_ctx(void) {
  return malloc(4 * 4 * 15 + 4);
}

void esp_aes128_setup_enc(void *ctxv, const uint8_t *key) {
  (void) rijndaelKeySetupEnc(ctxv, key, 128);
}

void esp_aes128_setup_dec(void *ctxv, const uint8_t *key) {
  rijndaelKeySetupDec(ctxv, key);
}

static void esp_aes128_encrypt(void *ctxv, const uint8_t *in, int len,
                               uint8_t *out) {
  while (len > 0) {
    rijndaelEncrypt(ctxv, 10, in, out);
    in += AES_BLOCK_SIZE;
    out += AES_BLOCK_SIZE;
    len -= AES_BLOCK_SIZE;
  }
}

static void esp_aes128_decrypt(void *ctxv, const uint8_t *in, int len,
                               uint8_t *out) {
  while (len > 0) {
    rijndaelDecrypt(ctxv, in, out);
    in += AES_BLOCK_SIZE;
    out += AES_BLOCK_SIZE;
    len -= AES_BLOCK_SIZE;
  }
}

static void esp_aes128_free_ctx(void *ctxv) {
  free(ctxv);
}

const kr_cipher_info *kr_aes128_cs_info(void) {
  static const kr_cipher_info aes128_cs_info = {
      AES_BLOCK_SIZE,     AES128_KEY_SIZE,      AES128_IV_SIZE,
      esp_aes128_new_ctx, esp_aes128_setup_enc, esp_aes128_setup_dec,
      esp_aes128_encrypt, esp_aes128_decrypt,   esp_aes128_free_ctx};
  return &aes128_cs_info;
}
#endif /* KR_EXT_AES */

#ifdef KR_EXT_RANDOM
int kr_get_random(uint8_t *out, size_t len) {
  return os_get_random(out, len) == 0;
}
#endif

/* For mbedTLS */
#else

#include "mbedtls/aes.h"

int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits) {
  if (keybits != 128) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
  rijndaelKeySetupEnc(ctx, key, 128);
  return 0;
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits) {
  if (keybits != 128) return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
  rijndaelKeySetupDec(ctx, key);
  return 0;
}

void mbedtls_aes_encrypt(mbedtls_aes_context *ctx,
                         const unsigned char input[16],
                         unsigned char output[16]) {
  rijndaelEncrypt(ctx, 10, input, output);
}

void mbedtls_aes_decrypt(mbedtls_aes_context *ctx,
                         const unsigned char input[16],
                         unsigned char output[16]) {
  rijndaelDecrypt(ctx, input, output);
}

/* os_get_random uses hardware RNG, so it's cool. */
int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  os_get_random(buf, len);
  (void) ctx;
  return 0;
}

#endif /* MG_SSL_IF == MG_SSL_IF_MBEDTLS */

#endif /* MG_ENABLE_SSL */
