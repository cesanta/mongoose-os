/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CONFIG_JS_H_
#define CS_FW_SRC_SJ_CONFIG_JS_H_

#ifdef SJ_ENABLE_JS

#include <stdbool.h>

#include "fw/src/sj_config.h"
#include "v7/v7.h"

v7_val_t sj_conf_mk_proxy(struct v7 *v7, const struct sj_conf_entry *schema,
                          void *cfg, bool read_only,
                          v7_cfunction_t *save_handler);

#endif /* SJ_ENABLE_JS */

#endif /* CS_FW_SRC_SJ_CONFIG_JS_H_ */
