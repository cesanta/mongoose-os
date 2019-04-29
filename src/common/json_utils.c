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

#include <string.h>

#include "common/json_utils.h"

void mg_json_emit_str(struct mbuf *b, const struct mg_str s, int quote) WEAK;
void mg_json_emit_str(struct mbuf *b, const struct mg_str s, int quote) {
  struct json_out out = JSON_OUT_MBUF(b);
  if (quote) mbuf_append(b, "\"", 1);
  json_escape(&out, s.p, s.len);
  if (quote) mbuf_append(b, "\"", 1);
}

int mg_json_printer_mbuf(struct json_out *, const char *, size_t) WEAK;
int mg_json_printer_mbuf(struct json_out *out, const char *buf, size_t len) {
  mbuf_append((struct mbuf *) out->u.data, buf, len);
  return len;
}
