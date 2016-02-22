/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef _ESP_MG_NET_IF_H_
#define _ESP_MG_NET_IF_H_

#ifndef RTOS_SDK

#include "v7/v7.h"
#include "mongoose/mongoose.h"

void mg_dispatch_v7_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                             v7_val_t args);

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr);

/* TODO(alashkin): Should we move these functions to mongoose interface? */
void mg_suspend();
void mg_resume();
int mg_is_suspended();

#endif /* !RTOS_SDK */

#endif /* _ESP_MG_NET_IF_H_ */
