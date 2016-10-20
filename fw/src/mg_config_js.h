/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CONFIG_JS_H_
#define CS_FW_SRC_MG_CONFIG_JS_H_

#if MG_ENABLE_JS

#include <stdbool.h>

#include "fw/src/mg_config.h"
#include "v7/v7.h"

v7_val_t mg_conf_mk_proxy(struct v7 *v7, const struct mg_conf_entry *schema,
                          void *cfg, bool read_only,
                          v7_cfunction_t *save_handler);

#endif /* MG_ENABLE_JS */

#endif /* CS_FW_SRC_MG_CONFIG_JS_H_ */
