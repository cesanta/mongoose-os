/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_

#include <stdbool.h>

#include "fw/platforms/cc3200/boot/lib/boot.h"

#ifdef __cplusplus
extern "C" {
#endif

void cc3200_main_task(void *arg);

extern int g_boot_cfg_idx;
extern struct boot_cfg g_boot_cfg;

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_MAIN_TASK_H_ */
