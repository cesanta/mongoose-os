/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MDNS_INTERNAL_H_
#define CS_FW_SRC_MGOS_MDNS_INTERNAL_H_

#include "mgos_features.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"

#include "mgos_mdns.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MDNS

enum mgos_init_result mgos_mdns_init(void);

#endif /* MGOS_ENABLE_MDNS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MDNS_INTERNAL_H_ */
