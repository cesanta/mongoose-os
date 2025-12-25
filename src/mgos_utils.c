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

#include "mgos_utils.h"

#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_hal.h"
#include "mgos_timers.h"

extern enum cs_log_level cs_log_level;
#if CS_ENABLE_STDIO
extern FILE *cs_log_file;
#endif

float mgos_rand_range(float from, float to) {
  return from + (((float) (to - from)) / RAND_MAX * rand());
}


size_t mbuf_remove_range(struct mbuf *mb, size_t i, size_t n) WEAK;
size_t mbuf_remove_range(struct mbuf *mb, size_t i, size_t n) {
  size_t ret = 0;
  if (n > 0 && n <= (mb->len - i)) {
    memmove(&mb->buf[i], &mb->buf[i] + n, mb->len - n);
    mb->len -= n;
    ret = n;
  }
  else if (n > 0 && n >= (mb->len - i)) {
    ret = mb->len - i;
    mb->len -= ret;
  }
  return ret;
}

#if CS_ENABLE_STDIO
/*
 * Intended for ffi
 */
void mgos_log(const char *filename, int line_no, int level, const char *msg) {
  if (cs_log_print_prefix((enum cs_log_level) level, filename, line_no)) {
    cs_log_printf("%s", msg);
  }
};
#endif
