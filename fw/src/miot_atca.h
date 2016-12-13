/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_ATCA_H_
#define CS_FW_SRC_MIOT_ATCA_H_

#include "fw/src/miot_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_ATCA
enum miot_init_result miot_atca_init(void);
#if MIOT_ENABLE_RPC && MIOT_ENABLE_ATCA_SERVICE
enum miot_init_result miot_atca_service_init(void);
#endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_ATCA_H_ */
