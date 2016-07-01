/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CONFIG_H_
#define CS_FW_SRC_SJ_CONFIG_H_

#include <stdbool.h>
#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

/*
 * NOTE(lsm): sj_conf_get_str() allocates *val on heap. Caller owns it.
 * NOTE(lsm): sj_conf_get_str() calls free() before assigning *val. Therefore,
 *            make sure that *val is set to NULL before the first call.
 */

/*
 * The usage pattern is this:
 * 1. Create an empty config struct at the beginning.
 * 2. Load the defaults.
 * 3. Then, apply overrides.
 *
 * When override is applied, previously allocated values are freed.
 * See ../test/unit_test.c for an example.
 */

bool sj_conf_check_access(const struct mg_str key, const char *acl);

enum sj_conf_type {
  CONF_TYPE_INT = 0,
  CONF_TYPE_BOOL = 1,
  CONF_TYPE_STRING = 2,
  CONF_TYPE_OBJECT = 3,
};

struct sj_conf_entry {
  enum sj_conf_type type;
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
bool sj_conf_parse(const char *json, const char *acl,
                   const struct sj_conf_entry *schema, void *cfg);

/*
 * Emit config in 'cfg' according to rules in 'schema'.
 * Keys are only emitted if their values are different from 'base'.
 * If 'base' is NULL then all keys are emitted.
 */
char *sj_conf_emit(const void *cfg, const void *base,
                   const struct sj_conf_entry *schema);

/*
 * Frees any resources allocated in 'cfg'.
 */
void sj_conf_free(const struct sj_conf_entry *schema, void *cfg);

#endif /* CS_FW_SRC_SJ_CONFIG_H_ */
