/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_GPIO_SERVICE_H_
#define CS_FW_SRC_MGOS_GPIO_SERVICE_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_RPC && MGOS_ENABLE_GPIO_SERVICE

#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result mgos_gpio_service_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_GPIO_SERVICE */
#endif /* CS_FW_SRC_MGOS_GPIO_SERVICE_H_ */
