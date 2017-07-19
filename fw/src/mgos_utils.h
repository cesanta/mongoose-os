/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_UTILS_H_
#define CS_FW_SRC_MGOS_UTILS_H_

#include "mgos_features.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Restart system after the specified number of milliseconds */
void mgos_system_restart_after(int delay_ms);

float mgos_rand_range(float from, float to);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UTILS_H_ */
