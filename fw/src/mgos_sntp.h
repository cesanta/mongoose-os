/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_SNTP_H_
#define CS_FW_SRC_MGOS_SNTP_H_

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_SNTP

/* Callback is invoked immediately after a time adjustment has been made,
 * with the change as the argument. */
typedef void (*mgos_sntp_time_change_cb)(void *arg, double delta);

void mgos_sntp_add_time_change_cb(mgos_sntp_time_change_cb cb, void *arg);

enum mgos_init_result mgos_sntp_init(void);

#endif /* MGOS_ENABLE_SNTP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_SNTP_H_ */
