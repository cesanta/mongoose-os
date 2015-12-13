#include "sj_timers.h"

#include <malloc.h>

#include "sj_v7_ext.h"

#include "FreeRTOS.h"
#include "timers.h"

extern struct v7* s_v7;

static void sj_timer_callback(TimerHandle_t t) {
  v7_val_t* cb = (v7_val_t*) pvTimerGetTimerID(t);
  xTimerDelete(t, 0);
  sj_invoke_cb0(s_v7, *cb);
  v7_disown(s_v7, cb);
  free(cb);
}

void sj_set_timeout(int msecs, v7_val_t* cb) {
  TimerHandle_t t =
      xTimerCreate("js_timeout", msecs * 1000 / configTICK_RATE_HZ, pdFALSE, cb,
                   sj_timer_callback);
  xTimerStart(t, 0);
}
