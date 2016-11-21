/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_ATCA_H_
#define CS_FW_SRC_MIOT_ATCA_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_ATCA

enum miot_init_result miot_atca_init(void);

#endif

#endif /* CS_FW_SRC_MIOT_ATCA_H_ */
