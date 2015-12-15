#ifndef SJ_CONFIG_H
#define SJ_CONFIG_H

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

int sj_conf_get_str(struct json_token *toks, const char *key, char **val);
int sj_conf_get_int(struct json_token *toks, const char *key, int *val);
int sj_conf_get_bool(struct json_token *toks, const char *key, int *val);

#endif
