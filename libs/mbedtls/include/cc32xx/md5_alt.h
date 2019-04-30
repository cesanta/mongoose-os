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

#pragma once

#include "cc32xx_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cc32xx_hash_ctx mbedtls_md5_context;

void mbedtls_md5_init(mbedtls_md5_context *ctx);
void mbedtls_md5_free(mbedtls_md5_context *ctx);
void mbedtls_md5_clone(mbedtls_md5_context *dst,
                       const mbedtls_md5_context *src);
int mbedtls_md5_starts_ret(mbedtls_md5_context *ctx);
int mbedtls_md5_update_ret(mbedtls_md5_context *ctx, const unsigned char *input,
                           size_t ilen);
int mbedtls_md5_finish_ret(mbedtls_md5_context *ctx, unsigned char output[32]);
int mbedtls_internal_md5_process(mbedtls_md5_context *ctx,
                                 const unsigned char data[64]);

#ifdef __cplusplus
}
#endif
