/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_UBJSERIALIZER_H_
#define CS_COMMON_UBJSERIALIZER_H_

#include <stdlib.h>

#if CS_ENABLE_UBJSON

#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* incremental UBJSON serializer */

/* data model */
struct ub_arr;
struct ub_bin;
struct ub_obj;
struct ub_str;

enum ubjson_type {
  UBJSON_TYPE_ARRAY,
  UBJSON_TYPE_BIN,
  UBJSON_TYPE_FALSE,
  UBJSON_TYPE_NULL,
  UBJSON_TYPE_NUMBER,
  UBJSON_TYPE_OBJECT,
  UBJSON_TYPE_STRING,
  UBJSON_TYPE_TRUE,
  UBJSON_TYPE_UNDEFINED,
};

typedef struct {
  enum ubjson_type kind;
  union {
    double n;
    struct ub_str *s;
    struct ub_arr *a;
    struct ub_bin *b;
    struct ub_obj *o;
  } val;
} ub_val_t;

struct ub_ctx;

typedef void (*ub_cb_t)(char *d, size_t l, int end, void *user_data);
typedef void (*ub_bin_cb_t)(struct ub_ctx *ctx, void *user_data);

struct ub_ctx *ub_ctx_new(void);
void ub_ctx_free(struct ub_ctx *ctx);
void ub_render(struct ub_ctx *ctx, ub_val_t root, ub_cb_t cb, void *user_data);

ub_val_t ub_create_array(struct ub_ctx *ctx);
ub_val_t ub_create_bin(struct ub_ctx *ctx, size_t n, ub_bin_cb_t cb,
                       void *user_data);
ub_val_t ub_create_boolean(int n);
ub_val_t ub_create_null(void);
ub_val_t ub_create_number(double n);
ub_val_t ub_create_object(struct ub_ctx *ctx);
ub_val_t ub_create_string(struct ub_ctx *ctx, const struct mg_str s);
ub_val_t ub_create_cstring(struct ub_ctx *ctx, const char *s);
ub_val_t ub_create_undefined(void);

int ub_is_bin(ub_val_t);
int ub_is_undefined(ub_val_t);

void ub_add_prop(struct ub_ctx *ctx, ub_val_t obj, const char *name,
                 ub_val_t val);
void ub_array_push(struct ub_ctx *ctx, ub_val_t a, ub_val_t val);

/* invoke this from the ub_bin_cb_t to send more chunks of the bin */
void ub_bin_send(struct ub_ctx *ctx, void *d, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_ENABLE_UBJSON */
#endif /* CS_COMMON_UBJSERIALIZER_H_ */
