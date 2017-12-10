/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_SNTP_INCLUDE_H_
#define CS_FW_SRC_MGOS_SNTP_INCLUDE_H_

#include "mgos_features.h"
#include "mgos_init.h"

#include "mgos_sntp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_SNTP

enum mgos_init_result mgos_sntp_init(void);

#endif /* MGOS_ENABLE_SNTP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_SNTP_INCLUDE_H_ */
