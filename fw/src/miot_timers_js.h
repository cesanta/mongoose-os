/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_TIMERS_JS_H_
#define CS_FW_SRC_MIOT_TIMERS_JS_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_JS

#include <stdint.h>

#include "fw/src/miot_timers.h"
#include "v7/v7.h"

struct v7;
void miot_timers_api_setup(struct v7 *v7);
miot_timer_id miot_set_js_timer(int msecs, int repeat, struct v7 *v7,
                                v7_val_t cb);

#endif /* MIOT_ENABLE_JS */

#endif /* CS_FW_SRC_MIOT_TIMERS_JS_H_ */
