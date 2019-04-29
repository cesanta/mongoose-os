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

#include <stdbool.h>
#include <stdint.h>

#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"

#include "mongoose.h"

void mg_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  mbedtls_md5_context md5_ctx;
  mbedtls_md5_init(&md5_ctx);
  mbedtls_md5_starts(&md5_ctx);
  for (i = 0; i < num_msgs; i++) {
    mbedtls_md5_update(&md5_ctx, msgs[i], msg_lens[i]);
  }
  mbedtls_md5_finish(&md5_ctx, digest);
  mbedtls_md5_free(&md5_ctx);
}

void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  mbedtls_sha1_context sha1_ctx;
  mbedtls_sha1_init(&sha1_ctx);
  mbedtls_sha1_starts(&sha1_ctx);
  for (i = 0; i < num_msgs; i++) {
    mbedtls_sha1_update(&sha1_ctx, msgs[i], msg_lens[i]);
  }
  mbedtls_sha1_finish(&sha1_ctx, digest);
  mbedtls_sha1_free(&sha1_ctx);
}
