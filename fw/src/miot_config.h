/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_CONFIG_H_
#define CS_FW_SRC_MIOT_CONFIG_H_

#include <stdbool.h>

#include "common/mbuf.h"
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * The usage pattern is this:
 * 1. Create an empty config struct at the beginning.
 * 2. Load the defaults.
 * 3. Then, apply overrides.
 *
 * When override is applied, previously allocated values are freed.
 * See ../test/unit_test.c for an example.
 */

bool miot_conf_check_access(const struct mg_str key, const char *acl);

enum miot_conf_type {
  CONF_TYPE_INT = 0,
  CONF_TYPE_BOOL = 1,
  CONF_TYPE_STRING = 2,
  CONF_TYPE_OBJECT = 3,
};

struct miot_conf_entry {
  enum miot_conf_type type;
  const char *key;
  union {
    int offset;
    int num_desc;
  };
};

/*
 * Parses config in 'json' into 'cfg' according to rules defined in 'schema' and
 * checking keys against 'acl'.
 */
bool miot_conf_parse(const struct mg_str json, const char *acl,
                     const struct miot_conf_entry *schema, void *cfg);

/*
 * Emit config in 'cfg' according to rules in 'schema'.
 * Keys are only emitted if their values are different from 'base'.
 * If 'base' is NULL then all keys are emitted.
 */
typedef void (*miot_conf_emit_cb_t)(struct mbuf *data, void *param);
void miot_conf_emit_cb(const void *cfg, const void *base,
                       const struct miot_conf_entry *schema, bool pretty,
                       struct mbuf *out, miot_conf_emit_cb_t cb,
                       void *cb_param);
bool miot_conf_emit_f(const void *cfg, const void *base,
                      const struct miot_conf_entry *schema, bool pretty,
                      const char *fname);

/*
 * Frees any resources allocated in 'cfg'.
 */
void miot_conf_free(const struct miot_conf_entry *schema, void *cfg);

const struct miot_conf_entry *miot_conf_find_schema_entry(
    const char *path, const struct miot_conf_entry *obj);

void miot_conf_set_str(char **vp, const char *v);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_CONFIG_H_ */
