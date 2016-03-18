/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_CC3200_CC3200_SJ_HAL_H_
#define CS_SMARTJS_PLATFORMS_CC3200_CC3200_SJ_HAL_H_

#include "v7/v7.h"

#define PROMPT_CHAR_EVENT 0
#define V7_INVOKE_EVENT 1
#define GPIO_INT_EVENT 2
struct sj_event {
  int type;
  void *data;
};

struct v7_invoke_event_data {
  v7_val_t func;
  v7_val_t this_obj;
  v7_val_t args;
};

size_t sj_get_heap_size();

#endif /* CS_SMARTJS_PLATFORMS_CC3200_CC3200_SJ_HAL_H_ */
