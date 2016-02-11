/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "sj_common.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"

void sj_init_common(struct v7 *v7) {
  sj_init_timers(v7);
  sj_init_v7_ext(v7);
}
