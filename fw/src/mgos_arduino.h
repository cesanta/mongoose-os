/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_ARDUINO_MGOS_ARDUINO_H_
#define CS_FW_SRC_ARDUINO_MGOS_ARDUINO_H_

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_ARDUINO_API

enum mgos_init_result mgos_arduino_init(void);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_ARDUINO_MGOS_ARDUINO_H_ */
