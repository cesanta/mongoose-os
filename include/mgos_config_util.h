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

/*
 * Low-level interface to the configuration infrastructure.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Return `true` if a given ACL allows `key` modification.
 *
 * ACL is a comma-separated list of globs, each glob might be additionally
 * prefixed with `+` (which is a no-op) or `-` (which means that matching keys
 * are NOT allowed).
 *
 * For glob syntax details, see `mg_match_prefix()`.
 *
 * Example:
 *
 * ```c
 * // Allow everything starting from "foo.", except "foo.bar":
 * const char *acl = "-foo.bar,+foo.*,-*";
 * mgos_conf_check_access(mg_mk_str("foo.bar"), acl); // false
 * mgos_conf_check_access(mg_mk_str("foo.qwe"), acl); // true
 * mgos_conf_check_access(mg_mk_str("foo.rty"), acl); // true
 * mgos_conf_check_access(mg_mk_str("hey"), acl);     // false
 * ```
 */
bool mgos_conf_check_access(const struct mg_str key, const char *acl);

/* Same as `mgos_conf_check_access()`, but `acl` is `struct mg_str`. */
bool mgos_conf_check_access_n(const struct mg_str key, struct mg_str acl);

/* Possible types of a configuration value. */
enum mgos_conf_type {
  CONF_TYPE_INT = 0,
  CONF_TYPE_BOOL = 1,
  CONF_TYPE_DOUBLE = 2,
  CONF_TYPE_STRING = 3,
  CONF_TYPE_OBJECT = 4,
  CONF_TYPE_UNSIGNED_INT = 5,
  CONF_TYPE_FLOAT = 6,
};

/* Configuration entry */
struct mgos_conf_entry {
  enum mgos_conf_type type;
  const char *key;
  uint16_t offset;
  uint16_t num_desc;
};

/* Generated during build */
struct mgos_config;

/*
 * Parses config `json` into `cfg` according to rules defined in `schema` and
 * checking keys against `acl`.
 */
bool mgos_conf_parse(const struct mg_str json, const char *acl,
                     struct mgos_config *cfg);

/*
 * Identical to `mgos_conf_parse()` but allows the caller to get an error
 * message in `msg` on parse or data type errors.
 *
 * If `msg` is not NULL it may contain heap-allocated eror message, which
 * is owned by the caller and has to be free()d.
 */
bool mgos_conf_parse_msg(const struct mg_str json, const char *acl,
                         struct mgos_config *cfg, char **msg);

/*
 * Parse a sub-section of the config.
 */
bool mgos_conf_parse_sub(const struct mg_str json,
                         const struct mgos_conf_entry *sub_schema, void *cfg);

bool mgos_conf_parse_sub_f(const char *fname,
                           const struct mgos_conf_entry *sub_schema,
                           const void *cfg);

bool mgos_conf_parse_sub_msg(const struct mg_str json,
                             const struct mgos_conf_entry *sub_schema,
                             const char *acl, void *cfg, char **msg);

/*
 * Callback for `mgos_conf_emit_cb` (see below); `data` is the emitted data and
 * `param` is user-defined param given to `mgos_conf_emit_cb`.
 */
typedef void (*mgos_conf_emit_cb_t)(struct mbuf *data, void *param);

/*
 * Emit config in `cfg` according to rules in `schema`.
 * Keys are only emitted if their values are different from `base`.
 * If `base` is NULL then all keys are emitted.
 *
 * If `pretty` is true, the output is prettified.
 *
 * If `out` is not NULL, output will be written there; otherwise an internal
 * mbuf will be allocated.
 *
 * If `cb` is not `NULL`, it'll be called with the resulting output and
 * `cb_param`.
 */
void mgos_conf_emit_cb(const void *cfg, const void *base,
                       const struct mgos_conf_entry *schema, bool pretty,
                       struct mbuf *out, mgos_conf_emit_cb_t cb,
                       void *cb_param);

/*
 * Like mgos_conf_emit_cb, but instead of writing the output in the provided
 * mbuf and/or calling user-provided callback, it writes the result into the
 * file with the given name `fname`.
 */
bool mgos_conf_emit_f(const void *cfg, const void *base,
                      const struct mgos_conf_entry *schema, bool pretty,
                      const char *fname);

/*
 * Like mgos_conf_emit_cb, but instead of writing the output in the provided
 * mbuf and/or calling user-provided callback, it forwards data into a json_out.
 */
bool mgos_conf_emit_json_out(const void *cfg, const void *base,
                             const struct mgos_conf_entry *schema, bool pretty,
                             struct json_out *out);

/*
 * Copies a config struct from src to dst.
 * The copy is independent and needs to be freed.
 */
bool mgos_conf_copy(const struct mgos_conf_entry *schema, const void *src,
                    void *dst);

/*
 * Frees any resources allocated in `cfg`.
 */
void mgos_conf_free(const struct mgos_conf_entry *schema, void *cfg);

/*
 * Finds a config schema entry by the "outer" entry (which has to describe an
 * object) and a path like "foo.bar.baz". If matching entry is not found,
 * returns `NULL`.
 */
const struct mgos_conf_entry *mgos_conf_find_schema_entry(
    const char *path, const struct mgos_conf_entry *schema);

/*
 * Like `mgos_conf_find_schema_entry()`, but takes the path as a
 * `struct mg_str`.
 */
const struct mgos_conf_entry *mgos_conf_find_schema_entry_s(
    const struct mg_str path, const struct mgos_conf_entry *obj);

/*
 * Get config value |key| in |cfg| according to |schema|.
 * Boolean and numeric values are stringified, objects are serialized as JSON.
 * Note: Returned value is heap-allocated and must be freed.
 */
bool mgos_config_get(const struct mg_str key, struct mg_str *value,
                     const void *cfg, const struct mgos_conf_entry *schema);

/*
 * Set config value |key| in |cfg| according to |schema|.
 * Boolean and numeric values are converted from string, objects are parsed as
 * JSON.
 */
bool mgos_config_set(const struct mg_str key, const struct mg_str value,
                     void *cfg, const struct mgos_conf_entry *schema,
                     bool free_strings);

/* Set string configuration entry. Frees current entry. */
void mgos_conf_set_str(const char **vp, const char *v);

/* Returns true if the string is NULL or empty. */
bool mgos_conf_str_empty(const char *s);

/* Copies a string if necessary. */
bool mgos_conf_copy_str(const char *s, const char **copy);

/* Frees a string if necessary. */
void mgos_conf_free_str(const char **sp);

/*
 * Returns a type of the value (this function is primarily for FFI)
 */
enum mgos_conf_type mgos_conf_value_type(struct mgos_conf_entry *e);

/*
 * Returns a string value from the config entry. If the value is empty,
 * returns NULL.
 */
const char *mgos_conf_value_string(const void *cfg,
                                   const struct mgos_conf_entry *e);

/*
 * Same as mgos_conf_value_string(), but returns an empty string instead of
 * NULL when the value is empty.
 */
const char *mgos_conf_value_string_nonnull(const void *cfg,
                                           const struct mgos_conf_entry *e);
/*
 * Returns an int or bool value from the config entry
 */
int mgos_conf_value_int(const void *cfg, const struct mgos_conf_entry *e);

/*
 * Returns a double value from the config entry
 */
double mgos_conf_value_double(const void *cfg, const struct mgos_conf_entry *e);

/*
 * Expands ? placeholders in str with characters from src, right to left.
 */
void mgos_expand_placeholders(const struct mg_str src, struct mg_str *str);

#ifdef __cplusplus
}
#endif /* __cplusplus */
