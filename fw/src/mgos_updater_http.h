/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * An updater implementation that fetches FW from the given URL.
 */

#ifndef CS_FW_SRC_MGOS_UPDATER_HTTP_H_
#define CS_FW_SRC_MGOS_UPDATER_HTTP_H_

#include <stdbool.h>
#include <stdint.h>

#include "mongoose/mongoose.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_updater_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_UPDATER
enum mgos_init_result mgos_updater_http_init(void);

void mgos_updater_http_start(struct update_context *ctx, const char *url);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_HTTP_H_ */
