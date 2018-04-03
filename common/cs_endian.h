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

#ifndef CS_COMMON_CS_ENDIAN_H_
#define CS_COMMON_CS_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * clang with std=-c99 uses __LITTLE_ENDIAN, by default
 * while for ex, RTOS gcc - LITTLE_ENDIAN, by default
 * it depends on __USE_BSD, but let's have everything
 */
#if !defined(BYTE_ORDER) && defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __LITTLE_ENDIAN
#endif /* BIG_ENDIAN */
#endif /* BYTE_ORDER */

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_CS_ENDIAN_H_ */
