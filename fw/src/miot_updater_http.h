/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * An updater implementation that fetches FW from the given URL.
 */

#ifndef CS_FW_SRC_MIOT_UPDATER_HTTP_H_
#define CS_FW_SRC_MIOT_UPDATER_HTTP_H_

#include <stdbool.h>
#include <stdint.h>

#include "mongoose/mongoose.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_updater_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_UPDATER
enum miot_init_result miot_updater_http_init(void);

void miot_updater_http_start(struct update_context *ctx, const char *url);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_UPDATER_HTTP_H_ */
