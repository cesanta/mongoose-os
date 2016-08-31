/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_MONGOOSE_H_
#define CS_FW_SRC_SJ_MONGOOSE_H_

#include "mongoose/mongoose.h"

extern struct mg_mgr sj_mgr;

void mongoose_init();
int mongoose_poll(int ms);
void mongoose_destroy();

typedef void (*mg_poll_cb_t)(void *cb_arg);
void mg_add_poll_cb(mg_poll_cb_t cb, void *cb_arg);
void mg_remove_poll_cb(mg_poll_cb_t cb, void *cb_arg);

/* HAL */

/* Schedule MG poll ASAP. Note: may be called from ISR context. */
void mongoose_schedule_poll();

#endif /* CS_FW_SRC_SJ_MONGOOSE_H_ */
