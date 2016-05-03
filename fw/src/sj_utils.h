/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SJ_UTILS_H
#define SJ_UTILS_H

#ifndef CS_DISABLE_JS

#include "v7/v7.h"
#include "mongoose/mongoose.h"

enum v7_err fill_ssl_connect_opts(struct v7 *v7, v7_val_t opts, int force_ssl,
                                  struct mg_connect_opts *copts);

#endif /* CS_DISABLE_JS */
#endif /* SJ_UTILS_H */
