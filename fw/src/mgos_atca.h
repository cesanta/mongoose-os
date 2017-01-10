/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_ATCA_H_
#define CS_FW_SRC_MGOS_ATCA_H_

#include "fw/src/mgos_features.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_ATCA
enum mgos_init_result mgos_atca_init(void);
bool mbedtls_atca_is_available();
#if MGOS_ENABLE_RPC && MGOS_ENABLE_ATCA_SERVICE
enum mgos_init_result mgos_atca_service_init(void);
#endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_ATCA_H_ */
