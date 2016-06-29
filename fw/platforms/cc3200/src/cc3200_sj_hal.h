/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_SJ_HAL_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_SJ_HAL_H_

#include "v7/v7.h"

size_t sj_get_heap_size();
size_t sj_get_free_heap_size();

void sj_system_restart(int exit_code);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_SJ_HAL_H_ */
