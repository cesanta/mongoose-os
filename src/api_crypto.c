/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "v7.h"
#include "fossa.h"

#ifndef V7_DISABLE_CRYPTO

static void base64_encode(const unsigned char *src, int src_len, char *dst) {
  static const char *b64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i, j, a, b, c;

  for (i = j = 0; i < src_len; i += 3) {
    a = src[i];
    b = i + 1 >= src_len ? 0 : src[i + 1];
    c = i + 2 >= src_len ? 0 : src[i + 2];

    dst[j++] = b64[a >> 2];
    dst[j++] = b64[((a & 3) << 4) | (b >> 4)];
    if (i + 1 < src_len) {
      dst[j++] = b64[(b & 15) << 2 | (c >> 6)];
    }
    if (i + 2 < src_len) {
      dst[j++] = b64[c & 63];
    }
  }
  while (j % 4 != 0) {
    dst[j++] = '=';
  }
  dst[j++] = '\0';
}

/* Convert one byte of encoded base64 input stream to 6-bit chunk */
static unsigned char from_b64(unsigned char ch) {
  /* Inverse lookup map */
  static const unsigned char tab[128] = {
      255, 255, 255, 255, 255, 255, 255, 255, /* 0 */
      255, 255, 255, 255, 255, 255, 255, 255, /* 8 */
      255, 255, 255, 255, 255, 255, 255, 255, /* 16 */
      255, 255, 255, 255, 255, 255, 255, 255, /* 24 */
      255, 255, 255, 255, 255, 255, 255, 255, /* 32 */
      255, 255, 255, 62,  255, 255, 255, 63,  /* 40 */
      52,  53,  54,  55,  56,  57,  58,  59,  /* 48 */
      60,  61,  255, 255, 255, 200, 255, 255, /* 56 '=' is 200, on index 61 */
      255, 0,   1,   2,   3,   4,   5,   6,   /* 64 */
      7,   8,   9,   10,  11,  12,  13,  14,  /* 72 */
      15,  16,  17,  18,  19,  20,  21,  22,  /* 80 */
      23,  24,  25,  255, 255, 255, 255, 255, /* 88 */
      255, 26,  27,  28,  29,  30,  31,  32,  /* 96 */
      33,  34,  35,  36,  37,  38,  39,  40,  /* 104 */
      41,  42,  43,  44,  45,  46,  47,  48,  /* 112 */
      49,  50,  51,  255, 255, 255, 255, 255, /* 120 */
  };
  return tab[ch & 127];
}


static void base64_decode(const unsigned char *s, int len, char *dst) {
  unsigned char a, b, c, d;
  while (len >= 4 && (a = from_b64(s[0])) != 255 &&
         (b = from_b64(s[1])) != 255 && (c = from_b64(s[2])) != 255 &&
         (d = from_b64(s[3])) != 255) {
    if (a == 200 || b == 200) break; /* '=' can't be there */
    *dst++ = a << 2 | b >> 4;
    if (c == 200) break;
    *dst++ = b << 4 | c >> 2;
    if (d == 200) break;
    *dst++ = c << 6 | d;
    s += 4;
    len -= 4;
  }
  *dst = 0;
}

static v7_val_t b64_transform(struct v7 *v7, v7_val_t this_obj, v7_val_t args,
                              void(func)(const unsigned char *, int, char *),
                              double mult) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);
  v7_val_t res = v7_create_undefined();

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t n;
    const char *s = v7_to_string(v7, &arg0, &n);
    char *buf = (char *) malloc(n * mult + 2);
    if (buf != NULL) {
      func((const unsigned char *) s, (int) n, buf);
      res = v7_create_string(v7, buf, strlen(buf), 1);
      free(buf);
    }
  }

  return res;
}

static v7_val_t Crypto_base64_decode(struct v7 *v7, v7_val_t this_obj,
                                  v7_val_t args) {
  return b64_transform(v7, this_obj, args, base64_decode, 0.75);
}

static v7_val_t Crypto_base64_encode(struct v7 *v7, v7_val_t this_obj,
                                  v7_val_t args) {
  return b64_transform(v7, this_obj, args, base64_encode, 1.5);
}

static void v7_md5(const char *data, size_t len, char buf[16]) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, (unsigned char *) data, len);
  MD5_Final((unsigned char *) buf, &ctx);
}

static void v7_sha1(const char *data, size_t len, char buf[20]) {
  SHA1_CTX ctx;
  SHA1Init(&ctx);
  SHA1Update(&ctx, (unsigned char *) data, len);
  SHA1Final((unsigned char *) buf, &ctx);
}

static void bin2str(char *to, const unsigned char *p, size_t len) {
  static const char *hex = "0123456789abcdef";

  for (; len--; p++) {
    *to++ = hex[p[0] >> 4];
    *to++ = hex[p[0] & 0x0f];
  }
  *to = '\0';
}

static v7_val_t Crypto_md5(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_to_string(v7, &arg0, &len);
    char buf[16];
    v7_md5(data, len, buf);
    return v7_create_string(v7, buf, sizeof(buf), 1);
  }
  return v7_create_null();
}

static v7_val_t Crypto_md5_hex(struct v7 *v7, v7_val_t this_obj,
                               v7_val_t args) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_to_string(v7, &arg0, &len);
    char hash[16], buf[sizeof(hash) * 2];
    v7_md5(data, len, hash);
    bin2str(buf, (unsigned char *) hash, sizeof(hash));
    return v7_create_string(v7, buf, sizeof(buf), 1);
  }
  return v7_create_null();
}

static v7_val_t Crypto_sha1(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_to_string(v7, &arg0, &len);
    char buf[20];
    v7_sha1(data, len, buf);
    return v7_create_string(v7, buf, sizeof(buf), 1);
  }
  return v7_create_null();
}

static v7_val_t Crypto_sha1_hex(struct v7 *v7, v7_val_t this_obj,
                                v7_val_t args) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_to_string(v7, &arg0, &len);
    char hash[20], buf[sizeof(hash) * 2];
    v7_sha1(data, len, hash);
    bin2str(buf, (unsigned char *) hash, sizeof(hash));
    return v7_create_string(v7, buf, sizeof(buf), 1);
  }
  return v7_create_null();
}
#endif

void init_crypto(struct v7 *v7) {
#ifndef V7_DISABLE_CRYPTO
  v7_val_t obj = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Crypto", 6, V7_PROPERTY_DONT_ENUM, obj);
  v7_set_method(v7, obj, "md5", Crypto_md5);
  v7_set_method(v7, obj, "md5_hex", Crypto_md5_hex);
  v7_set_method(v7, obj, "sha1", Crypto_sha1);
  v7_set_method(v7, obj, "sha1_hex", Crypto_sha1_hex);
  v7_set_method(v7, obj, "base64_encode", Crypto_base64_encode);
  v7_set_method(v7, obj, "base64_decode", Crypto_base64_decode);
#else
  (void) v7;
#endif
}
