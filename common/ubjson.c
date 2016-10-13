/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if CS_ENABLE_UBJSON

#include "common/ubjson.h"

void cs_ubjson_emit_null(struct mbuf *buf) {
  mbuf_append(buf, "Z", 1);
}

void cs_ubjson_emit_boolean(struct mbuf *buf, int v) {
  mbuf_append(buf, v ? "T" : "F", 1);
}

void cs_ubjson_emit_int8(struct mbuf *buf, int8_t v) {
  mbuf_append(buf, "i", 1);
  mbuf_append(buf, &v, 1);
}

void cs_ubjson_emit_uint8(struct mbuf *buf, uint8_t v) {
  mbuf_append(buf, "U", 1);
  mbuf_append(buf, &v, 1);
}

void cs_ubjson_emit_int16(struct mbuf *buf, int16_t v) {
  uint8_t b[1 + sizeof(uint16_t)];
  b[0] = 'I';
  b[1] = ((uint16_t) v) >> 8;
  b[2] = ((uint16_t) v) & 0xff;
  mbuf_append(buf, b, 1 + sizeof(uint16_t));
}

static void encode_uint32(uint8_t *b, uint32_t v) {
  b[0] = (v >> 24) & 0xff;
  b[1] = (v >> 16) & 0xff;
  b[2] = (v >> 8) & 0xff;
  b[3] = v & 0xff;
}

void cs_ubjson_emit_int32(struct mbuf *buf, int32_t v) {
  uint8_t b[1 + sizeof(uint32_t)];
  b[0] = 'l';
  encode_uint32(&b[1], (uint32_t) v);
  mbuf_append(buf, b, 1 + sizeof(uint32_t));
}

static void encode_uint64(uint8_t *b, uint64_t v) {
  b[0] = (v >> 56) & 0xff;
  b[1] = (v >> 48) & 0xff;
  b[2] = (v >> 40) & 0xff;
  b[3] = (v >> 32) & 0xff;
  b[4] = (v >> 24) & 0xff;
  b[5] = (v >> 16) & 0xff;
  b[6] = (v >> 8) & 0xff;
  b[7] = v & 0xff;
}

void cs_ubjson_emit_int64(struct mbuf *buf, int64_t v) {
  uint8_t b[1 + sizeof(uint64_t)];
  b[0] = 'L';
  encode_uint64(&b[1], (uint64_t) v);
  mbuf_append(buf, b, 1 + sizeof(uint64_t));
}

void cs_ubjson_emit_autoint(struct mbuf *buf, int64_t v) {
  if (v >= INT8_MIN && v <= INT8_MAX) {
    cs_ubjson_emit_int8(buf, (int8_t) v);
  } else if (v >= 0 && v <= 255) {
    cs_ubjson_emit_uint8(buf, (uint8_t) v);
  } else if (v >= INT16_MIN && v <= INT16_MAX) {
    cs_ubjson_emit_int16(buf, (int32_t) v);
  } else if (v >= INT32_MIN && v <= INT32_MAX) {
    cs_ubjson_emit_int32(buf, (int32_t) v);
  } else if (v >= INT64_MIN && v <= INT64_MAX) {
    cs_ubjson_emit_int64(buf, (int64_t) v);
  } else {
    /* TODO(mkm): use "high-precision" stringified type */
    abort();
  }
}

void cs_ubjson_emit_float32(struct mbuf *buf, float v) {
  uint32_t n;
  uint8_t b[1 + sizeof(uint32_t)];
  b[0] = 'd';
  memcpy(&n, &v, sizeof(v));
  encode_uint32(&b[1], n);
  mbuf_append(buf, b, 1 + sizeof(uint32_t));
}

void cs_ubjson_emit_float64(struct mbuf *buf, double v) {
  uint64_t n;
  uint8_t b[1 + sizeof(uint64_t)];
  b[0] = 'D';
  memcpy(&n, &v, sizeof(v));
  encode_uint64(&b[1], n);
  mbuf_append(buf, b, 1 + sizeof(uint64_t));
}

void cs_ubjson_emit_autonumber(struct mbuf *buf, double v) {
  int64_t i = (int64_t) v;
  if ((double) i == v) {
    cs_ubjson_emit_autoint(buf, i);
  } else {
    cs_ubjson_emit_float64(buf, v);
  }
}

void cs_ubjson_emit_size(struct mbuf *buf, size_t v) {
/* TODO(mkm): use "high-precision" stringified type */

#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX != INT32_MAX)
  /*
   * This assert expression is always true on 32-bit system,
   * shutting up compiler
   */
  assert((uint64_t) v < INT64_MAX);
#endif

  cs_ubjson_emit_autoint(buf, (int64_t) v);
}

void cs_ubjson_emit_string(struct mbuf *buf, const char *s, size_t len) {
  mbuf_append(buf, "S", 1);
  cs_ubjson_emit_size(buf, len);
  mbuf_append(buf, s, len);
}

void cs_ubjson_emit_bin_header(struct mbuf *buf, size_t len) {
  mbuf_append(buf, "[$U#", 4);
  cs_ubjson_emit_size(buf, len);
}

void cs_ubjson_emit_bin(struct mbuf *buf, const char *s, size_t len) {
  cs_ubjson_emit_bin_header(buf, len);
  mbuf_append(buf, s, len);
}

void cs_ubjson_open_object(struct mbuf *buf) {
  mbuf_append(buf, "{", 1);
}

void cs_ubjson_emit_object_key(struct mbuf *buf, const char *s, size_t len) {
  cs_ubjson_emit_size(buf, len);
  mbuf_append(buf, s, len);
}

void cs_ubjson_close_object(struct mbuf *buf) {
  mbuf_append(buf, "}", 1);
}

void cs_ubjson_open_array(struct mbuf *buf) {
  mbuf_append(buf, "[", 1);
}

void cs_ubjson_close_array(struct mbuf *buf) {
  mbuf_append(buf, "]", 1);
}

#else
void cs_ubjson_dummy();
#endif
