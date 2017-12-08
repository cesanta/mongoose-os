/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Low-level interface to the configuration infrastructure.
 */

#ifndef CS_FW_SRC_MGOS_CONFIG_H_
#define CS_FW_SRC_MGOS_CONFIG_H_

#include <stdbool.h>

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Return `true` if a given ACL allows `key` modification. */
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
};

/* Configuration entry */
struct mgos_conf_entry {
  enum mgos_conf_type type;
  const char *key;
  union {
    int offset;
    int num_desc;
  };
};

/*
 * Parses config `json` into `cfg` according to rules defined in `schema` and
 * checking keys against `acl`.
 */
bool mgos_conf_parse(const struct mg_str json, const char *acl,
                     const struct mgos_conf_entry *schema, void *cfg);

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
 * Frees any resources allocated in `cfg`.
 */
void mgos_conf_free(const struct mgos_conf_entry *schema, void *cfg);

/*
 * Finds a config schema entry by the "outer" entry (which has to describe an
 * object) and a path like "foo.bar.baz". If matching entry is not found,
 * returns `NULL`.
 */
const struct mgos_conf_entry *mgos_conf_find_schema_entry(
    const char *path, const struct mgos_conf_entry *obj);

/*
 * Like `mgos_conf_find_schema_entry()`, but takes the path as a `strct
 * mg_str`.
 */
const struct mgos_conf_entry *mgos_conf_find_schema_entry_s(
    const struct mg_str path, const struct mgos_conf_entry *obj);

/* Set string configuration entry. Frees current entry. */
void mgos_conf_set_str(char **vp, const char *v);

/* Returns true if the string is NULL or empty. */
bool mgos_conf_str_empty(const char *s);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_CONFIG_H_ */
