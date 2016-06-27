/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_

void main_task(void *arg);

typedef void (*cb_t)(void *arg);
void invoke_cb(cb_t cb, void *arg);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_ */
