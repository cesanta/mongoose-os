/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * SNTP API - see https://en.wikipedia.org/wiki/Network_Time_Protocol for
 * the background information.
 *
 * SNTP is required when a device needs a correct current time set,
 * e.g. for TLS communication. That means that SNTP is a must - with
 * exceptions of not connected devices or devices which can handle current
 * time via some other way.
 */

#ifndef CS_FW_SRC_MGOS_SNTP_H_
#define CS_FW_SRC_MGOS_SNTP_H_

#include "mgos_features.h"
#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_SNTP

/* Time change callback signature, `delta` is the number of seconds. */
typedef void (*mgos_sntp_time_change_cb)(void *arg, double delta);

/* Register callback for time change. */
void mgos_sntp_add_time_change_cb(mgos_sntp_time_change_cb cb, void *arg);

enum mgos_init_result mgos_sntp_init(void);

#endif /* MGOS_ENABLE_SNTP */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_SNTP_H_ */
