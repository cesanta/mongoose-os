/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include <string.h>

#include "common/json_utils.h"

void mg_json_emit_str(struct mbuf *b, const struct mg_str s, int quote) WEAK;
void mg_json_emit_str(struct mbuf *b, const struct mg_str s, int quote) {
  if (quote) mbuf_append(b, "\"", 1);
  /* TODO(rojer): JSON escaping. */
  if (s.len != 0) mbuf_append(b, s.p, s.len);
  if (quote) mbuf_append(b, "\"", 1);
}

int mg_json_printer_mbuf(struct json_out *, const char *, size_t) WEAK;
int mg_json_printer_mbuf(struct json_out *out, const char *buf, size_t len) {
  mbuf_append((struct mbuf *) out->u.data, buf, len);
  return len;
}
