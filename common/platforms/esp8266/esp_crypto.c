/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "krypton/krypton.h"

/*
 * Krypton API shims to crypto functions in ROM/SDK.
 *
 * SDK uses hostapd/wpa_supplicant code and thus brings its crypto with it.
 * We exploit this fact to reduce our footprint / speed up crypto (some of it is
 * in ROM, which is faster than flash).
 *
 * You can find hostapd/wpa_supplicant code here: https://w1.fi/cgit/
 */

/* MD5 */
#ifdef KR_EXT_MD5
extern int md5_vector(size_t num_msgs, const u8 *msgs[], const size_t *msg_lens,
                      uint8_t *digest);
void kr_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) {
  (void) md5_vector(num_msgs, msgs, msg_lens, digest);
}
#endif

/* SHA1 */
#ifdef KR_EXT_SHA1
extern int sha1_vector(size_t num_msgs, const u8 *msgs[],
                       const size_t *msg_lens, uint8_t *digest);
void kr_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  (void) sha1_vector(num_msgs, msgs, msg_lens, digest);
}
#endif

#ifdef KR_EXT_AES
/*
 * AES128
 *
 * For some reason, decryption is in ROM and encryption is supplied by SDK.
 * Moreover, the variant in ROM only supports 128 bits while the variant in SDK
 * supports 192 and 256 bit encryption.
 * In other words, code in ROM is from before commit
 * d140db6adf7b3b439f71e1ac2c72e637157bfc4a and SDK code is after.
 * I'm pretty sure it means AES 192 and 256 support won't actually work,
 * but hey, who am I to judge. :)
 */

#define AES_BLOCK_SIZE 16
#define AES128_KEY_SIZE 16
#define AES128_IV_SIZE 16

/* These are new, supplied by the SDK. */
int rijndaelKeySetupEnc(void *ctx, const uint8_t *key, int bits);
void rijndaelEncrypt(void *ctx, int rounds, const uint8_t *in, uint8_t *out);

/* These are old, in ROM - notice no "rounds" and "bits" params. */
void rijndaelKeySetupDec(void *ctx, const uint8_t *key);
void rijndaelDecrypt(void *ctx, const uint8_t *in, uint8_t *out);

static void *esp_aes128_new_ctx() {
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

/* Krypton API function. */
const kr_cipher_info *kr_aes128_cs_info() {
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
