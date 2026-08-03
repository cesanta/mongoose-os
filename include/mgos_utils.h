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

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define CS_CTASSERT_JOIN(a, b) a##b
#define CS_CTASSERT(pred, msg) \
  extern char CS_CTASSERT_JOIN(ASSERTION_FAILED_, msg)[!!(pred) ? 1 : -1]

#ifndef UNUSED_ARG
#define UNUSED_ARG __attribute__((unused))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Restart system after the specified number of milliseconds */
void mgos_system_restart_after(int delay_ms);

/* Return random number in a given range. */
float mgos_rand_range(float from, float to);

/* Removes 'data_size' bytes from index of the buffer */
size_t mbuf_remove_range(struct mbuf *, size_t index, size_t data_size);

#ifdef __cplusplus
}
#endif
