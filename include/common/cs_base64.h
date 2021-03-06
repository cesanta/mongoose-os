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

#ifndef CS_COMMON_CS_BASE64_H_
#define CS_COMMON_CS_BASE64_H_

#ifndef DISABLE_BASE64
#define DISABLE_BASE64 0
#endif

#if !DISABLE_BASE64

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cs_base64_putc_t)(char, void *);

struct cs_base64_ctx {
  /* cannot call it putc because it's a macro on some environments */
  cs_base64_putc_t b64_putc;
  unsigned char chunk[3];
  int chunk_size;
  void *user_data;
};

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t putc,
                    void *user_data);
void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len);
void cs_base64_finish(struct cs_base64_ctx *ctx);

void cs_base64_encode(const unsigned char *src, int src_len, char *dst, int *enc_len);
void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len);

/*
 * Decodes a base64 string `s` length `len` into `dst`.
 * `dst` must have enough space to hold the result.
 * `*dec_len` will contain the resulting length of the string in `dst`
 * while return value will return number of processed bytes in `src`.
 * Return value == len indicates successful processing of all the data.
 */
int cs_base64_decode(const unsigned char *s, int len, char *dst, int *dec_len);

#ifdef __cplusplus
}
#endif

#endif /* DISABLE_BASE64 */

#endif /* CS_COMMON_CS_BASE64_H_ */
