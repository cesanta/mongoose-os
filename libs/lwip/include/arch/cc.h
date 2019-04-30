/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
// For PPP
#include "mbedtls/des.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int sys_prot_t;

#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#define LWIP_PLATFORM_ASSERT(x)                                       \
  do {                                                                \
    printf("Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, \
           __FILE__);                                                 \
  } while (0)

#define LWIP_RAND() ((u32_t) rand())

#ifdef __cplusplus
}
#endif
