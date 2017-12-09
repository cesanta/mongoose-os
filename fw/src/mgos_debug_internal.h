/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_DEBUG_INTERNAL_H_
#define CS_FW_SRC_MGOS_DEBUG_INTERNAL_H_

#include "mgos_debug.h"

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result mgos_debug_init(void);
enum mgos_init_result mgos_debug_uart_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_DEBUG_INTERNAL_H_ */
