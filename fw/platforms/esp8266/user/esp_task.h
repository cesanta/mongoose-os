/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_TASK_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_TASK_H_

#include "mongoose/mongoose.h"

#include "fw/src/mg_features.h"

#if MG_ENABLE_JS

#include "v7/v7.h"

void mg_dispatch_v7_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                             v7_val_t args);
#endif

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr);

void mg_lwip_set_keepalive_params(struct mg_connection *nc, int idle,
                                  int interval, int count);

void esp_mg_task_init();

/* TODO(alashkin): Should we move these functions to mongoose interface? */
void mg_suspend(void);
void mg_resume(void);
int mg_is_suspended(void);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_TASK_H_ */
