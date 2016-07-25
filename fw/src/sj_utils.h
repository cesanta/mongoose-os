/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_UTILS_H_
#define CS_FW_SRC_SJ_UTILS_H_

#ifdef SJ_ENABLE_JS

#include "v7/v7.h"
#include "mongoose/mongoose.h"

enum v7_err fill_ssl_connect_opts(struct v7 *v7, v7_val_t opts, int force_ssl,
                                  struct mg_connect_opts *copts);

#endif /* SJ_ENABLE_JS */
#endif /* CS_FW_SRC_SJ_UTILS_H_ */
