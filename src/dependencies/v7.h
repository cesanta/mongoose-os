/*
 * Copyright (c) 2013-2014 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http: *www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <http://cesanta.com/products.html>.
 */

#ifndef V7_HEADER_INCLUDED
#define V7_HEADER_INCLUDED

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stddef.h> /* For size_t */
#include <stdio.h>  /* For FILE */

#define V7_VERSION "1.0"

enum v7_err { V7_OK, V7_SYNTAX_ERROR, V7_EXEC_EXCEPTION };

struct v7;     /* Opaque structure. V7 engine handler. */
struct v7_val; /* Opaque structure. Holds V7 value, which has v7_type type. */

#if (defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)) || \
    (defined(_MSC_VER) && _MSC_VER <= 1200)
#define V7_WINDOWS
#endif

#ifdef V7_WINDOWS
typedef unsigned __int64 uint64_t;
#else
#include <inttypes.h>
#endif
typedef uint64_t v7_val_t;

typedef v7_val_t (*v7_cfunction_t)(struct v7 *, v7_val_t, v7_val_t);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct v7 *v7_create(void);
void v7_destroy(struct v7 *);
enum v7_err v7_exec(struct v7 *, v7_val_t *, const char *str);
enum v7_err v7_exec_file(struct v7 *, v7_val_t *, const char *path);
enum v7_err v7_exec_with(struct v7 *, v7_val_t *, const char *str, v7_val_t);

void v7_gc(struct v7 *);

v7_val_t v7_create_object(struct v7 *v7);
v7_val_t v7_create_array(struct v7 *v7);
v7_val_t v7_create_cfunction(v7_cfunction_t func);
v7_val_t v7_create_number(double num);
v7_val_t v7_create_boolean(int is_true);
v7_val_t v7_create_null(void);
v7_val_t v7_create_undefined(void);
v7_val_t v7_create_string(struct v7 *v7, const char *, size_t, int);
v7_val_t v7_create_regexp(struct v7 *, const char *, size_t, const char *,
                          size_t);
v7_val_t v7_create_foreign(void *);
v7_val_t v7_create_cfunction_object(struct v7 *, v7_cfunction_t, int nargs);

int v7_is_object(v7_val_t);
int v7_is_function(v7_val_t);
int v7_is_cfunction(v7_val_t);
int v7_is_string(v7_val_t);
int v7_is_boolean(v7_val_t);
int v7_is_double(v7_val_t);
int v7_is_null(v7_val_t);
int v7_is_undefined(v7_val_t);
int v7_is_regexp(v7_val_t);
int v7_is_foreign(v7_val_t);

void *v7_to_foreign(v7_val_t);
int v7_to_boolean(v7_val_t);
double v7_to_double(v7_val_t);
v7_cfunction_t v7_to_cfunction(v7_val_t);
const char *v7_to_string(struct v7 *, v7_val_t *, size_t *);

v7_val_t v7_get_global_object(struct v7 *);
v7_val_t v7_get(struct v7 *v7, v7_val_t obj, const char *name, size_t len);
char *v7_to_json(struct v7 *, v7_val_t, char *, size_t);
int v7_is_true(struct v7 *v7, v7_val_t v);
v7_val_t v7_apply(struct v7 *, v7_val_t, v7_val_t, v7_val_t);
void v7_throw(struct v7 *, const char *, ...);

#define V7_PROPERTY_READ_ONLY 1
#define V7_PROPERTY_DONT_ENUM 2
#define V7_PROPERTY_DONT_DELETE 4
#define V7_PROPERTY_HIDDEN 8
#define V7_PROPERTY_GETTER 16
#define V7_PROPERTY_SETTER 32
int v7_set(struct v7 *v7, v7_val_t obj, const char *, size_t,
           unsigned int attrs, v7_val_t val);
#define v7_set_method(v7, obj, name, func)                         \
  v7_set((v7), (obj), (name), strlen(name), V7_PROPERTY_DONT_ENUM, \
         v7_create_cfunction_object((v7), (func), -1))

unsigned long v7_array_length(struct v7 *v7, v7_val_t arr);
int v7_array_set(struct v7 *v7, v7_val_t arr, unsigned long index, v7_val_t v);
int v7_array_push(struct v7 *, v7_val_t arr, v7_val_t v);
v7_val_t v7_array_get(struct v7 *, v7_val_t arr, unsigned long index);

void v7_compile(const char *code, int binary, FILE *fp);
int v7_main(int argc, char *argv[], void (*init_func)(struct v7 *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* V7_HEADER_INCLUDED */
