/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_JSON_UTILS_H_
#define CS_COMMON_JSON_UTILS_H_

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/platform.h"
#include "frozen/frozen.h"

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
      { (void *) mbuf_addr, 0, 0 } \
    }                              \
  }

#endif /* CS_COMMON_JSON_UTILS_H_ */
