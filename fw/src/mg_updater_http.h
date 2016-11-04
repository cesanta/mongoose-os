/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * An updater implementation that fetches FW from the given URL.
 */

#ifndef CS_FW_SRC_MG_UPDATER_HTTP_H_
#define CS_FW_SRC_MG_UPDATER_HTTP_H_

#include <stdbool.h>
#include <stdint.h>

#include "mongoose/mongoose.h"
#include "fw/src/mg_init.h"
#include "fw/src/mg_updater_common.h"

#if MG_ENABLE_UPDATER
enum mg_init_result mg_updater_http_init(void);

void mg_updater_http_start(struct update_context *ctx, const char *url);
#endif

#endif /* CS_FW_SRC_MG_UPDATER_HTTP_H_ */
