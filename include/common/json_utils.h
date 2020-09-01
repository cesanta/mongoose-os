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

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/platform.h"
#include "frozen.h"

#ifdef __cplusplus
extern "C" {
#endif

int mg_json_printer_mbuf(struct json_out *, const char *, size_t);

/* Add quoted string into mbuf */
void mg_json_emit_str(struct mbuf *b, const struct mg_str s, int quote);

/*
 * This macro is intended to use with Frozen
 * json_printf function to print JSON
 * into mbuf
 */
#define JSON_OUT_MBUF(mbuf_addr)   \
  {                                \
    mg_json_printer_mbuf, {        \
      { (char *) mbuf_addr, 0, 0 } \
    }                              \
  }

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string>

namespace mgos {

std::string JSONPrintfString(const char *fmt, ...);

}  // namespace mgos
#endif
