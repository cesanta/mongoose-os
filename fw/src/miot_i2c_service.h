/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_I2C_SERVICE_H_
#define CS_FW_SRC_MIOT_I2C_SERVICE_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_I2C && MIOT_ENABLE_RPC && MIOT_ENABLE_I2C_SERVICE

#include "fw/src/miot_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum miot_init_result miot_i2c_service_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_I2C && MIOT_ENABLE_RPC && MIOT_ENABLE_I2C_SERVICE */
#endif /* CS_FW_SRC_MIOT_I2C_SERVICE_H_ */
