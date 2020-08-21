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

#include <stdbool.h>
#include <stdint.h>

#ifdef MGOS_HAVE_MBEDTLS
#include "mbedtls/md.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Copy a file */
bool mgos_file_copy(const char *from, const char *to);

#ifdef MGOS_HAVE_MBEDTLS
/* Compute file's digest. *digest must have enough space for the digest type. */
bool mgos_file_digest(const char *fname, mbedtls_md_type_t dt, uint8_t *digest);

/* Copy the file if target does not exist or is different. */
bool mgos_file_copy_if_different(const char *from, const char *to);
#endif  // MGOS_HAVE_MBEDTLS

#ifdef __cplusplus
}
#endif
