/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_CONFIG_H_
#define CS_SMARTJS_SRC_SJ_CONFIG_H_

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

int sj_conf_get_str(struct json_token *toks, const char *key, const char *acl,
                    char **val);
int sj_conf_get_int(struct json_token *toks, const char *key, const char *acl,
                    int *val);
int sj_conf_get_bool(struct json_token *toks, const char *key, const char *acl,
                     int *val);
void sj_conf_emit_str(struct mbuf *b, const char *prefix, const char *s,
                      const char *suffix);
void sj_conf_emit_int(struct mbuf *b, int v);

int sj_conf_check_access(const char *key, const char *acl);

#endif /* CS_SMARTJS_SRC_SJ_CONFIG_H_ */
