// Copyright (c) 2013-2014 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <http://cesanta.com/products.html>.

#ifndef V7_HEADER_INCLUDED
#define  V7_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define V7_VERSION "1.0"

struct v7;      // Opaque structure. V7 engine handler.
struct v7_val;  // Opaque structure. Holds V7 value, which has v7_type type.

enum v7_type {
  V7_TYPE_UNDEF, V7_TYPE_NULL, V7_TYPE_BOOL, V7_TYPE_STR, V7_TYPE_NUM,
  V7_TYPE_OBJ, V7_NUM_TYPES
};

enum v7_err {
  V7_OK, V7_ERROR, V7_EVAL_ERROR, V7_RANGE_ERROR, V7_REFERENCE_ERROR,
  V7_SYNTAX_ERROR, V7_TYPE_ERROR, V7_URI_ERROR,
  V7_OUT_OF_MEMORY, V7_INTERNAL_ERROR, V7_STACK_OVERFLOW, V7_STACK_UNDERFLOW,
  V7_CALLED_NON_FUNCTION, V7_NOT_IMPLEMENTED, V7_NUM_ERRORS
};

// This structure is passed as an argument to the C/JS glue function
struct v7_c_func_arg {
  struct v7 *v7;
  struct v7_val *this_obj;
  struct v7_val **args;
  int num_args;
  int called_as_constructor;
};
typedef enum v7_err (*v7_func_t)(struct v7_c_func_arg *arg);


struct v7 *v7_create(void);       // Creates and initializes V7 engine
void v7_destroy(struct v7 **);    // Cleanes up and deallocates V7 engine

struct v7_val *v7_exec(struct v7 *, const char *str);         // Executes string
struct v7_val *v7_exec_file(struct v7 *, const char *path);   // Executes file

struct v7_val *v7_global(struct v7 *);  // Returns global obj (root namespace)
char *v7_stringify(const struct v7_val *v, char *buf, int bsiz);
const char *v7_get_error_string(const struct v7 *);  // Returns error string
int v7_is_true(const struct v7_val *);
void v7_copy(struct v7 *v7, struct v7_val *from, struct v7_val *to);

enum v7_err v7_set(struct v7 *, struct v7_val *, const char *, struct v7_val *);
enum v7_err v7_del(struct v7 *, struct v7_val *obj, const char *key);
struct v7_val *v7_get(struct v7_val *obj, const char *key);

struct v7_val *v7_call(struct v7 *v7, struct v7_val *this_obj, int num_args);

struct v7_val *v7_push_number(struct v7 *, double num);
struct v7_val *v7_push_bool(struct v7 *, int is_true);
struct v7_val *v7_push_string(struct v7 *, const char *str, unsigned long, int);
struct v7_val *v7_push_new_object(struct v7 *);
struct v7_val *v7_push_val(struct v7 *, struct v7_val *);
struct v7_val *v7_push_func(struct v7 *, v7_func_t);

enum v7_type v7_type(const struct v7_val *);
double v7_number(const struct v7_val *);
const char *v7_string(const struct v7_val *, unsigned long *len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // V7_HEADER_INCLUDED
