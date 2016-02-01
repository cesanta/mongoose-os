/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "sj_timers.h"

#include <malloc.h>

#include "sj_v7_ext.h"

#include "FreeRTOS.h"
#include "timers.h"

extern struct v7 *s_v7;

static void sj_timer_callback(TimerHandle_t t) {
  v7_val_t *cb = (v7_val_t *) pvTimerGetTimerID(t);
  xTimerDelete(t, 0);

  /* Schedule the callback invocation ASAP */
  sj_invoke_cb0(s_v7, *cb);

  /*
   * Disown and free the callback value which was allocated and owned in
   * `global_set_timeout()`
   */
  v7_disown(s_v7, cb);
  free(cb);
}

void sj_set_timeout(int msecs, v7_val_t *cb) {
  TimerHandle_t t =
      xTimerCreate("js_timeout", msecs * 1000 / configTICK_RATE_HZ, pdFALSE, cb,
                   sj_timer_callback);
  xTimerStart(t, 0);
}
