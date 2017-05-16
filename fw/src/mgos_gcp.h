/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_GCP_H_
#define CS_FW_SRC_MGOS_GCP_H_

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"

#if MGOS_ENABLE_GCP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result mgos_gcp_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_GCP */

#endif /* CS_FW_SRC_MGOS_GCP_H_ */
