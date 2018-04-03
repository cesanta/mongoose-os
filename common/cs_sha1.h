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

#ifndef CS_COMMON_SHA1_H_
#define CS_COMMON_SHA1_H_

#ifndef CS_DISABLE_SHA1
#define CS_DISABLE_SHA1 0
#endif

#if !CS_DISABLE_SHA1

#include "common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
} cs_sha1_ctx;

void cs_sha1_init(cs_sha1_ctx *);
void cs_sha1_update(cs_sha1_ctx *, const unsigned char *data, uint32_t len);
void cs_sha1_final(unsigned char digest[20], cs_sha1_ctx *);
void cs_hmac_sha1(const unsigned char *key, size_t key_len,
                  const unsigned char *text, size_t text_len,
                  unsigned char out[20]);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_DISABLE_SHA1 */

#endif /* CS_COMMON_SHA1_H_ */
