/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_DEBUG_HAL_H_
#define CS_FW_SRC_MGOS_DEBUG_HAL_H_

#include <stdlib.h>

#include "common/mg_str.h"

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result mgos_debug_udp_init(const char *dst);

void mgos_debug_udp_send(const struct mg_str prefix, const struct mg_str data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_DEBUG_HAL_H_ */
