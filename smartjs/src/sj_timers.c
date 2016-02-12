/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "sj_timers.h"

#include <stdlib.h>

#include "sj_v7_ext.h"
#include "sj_common.h"

/* Currently can only handle one timer */
SJ_PRIVATE enum v7_err global_set_timeout(struct v7 *v7, v7_val_t *res) {
  v7_val_t msecsv = v7_arg(v7, 1);
  int msecs;
  (void) res;

  if (!v7_is_callable(v7, v7_arg(v7, 0))) {
    printf("cb is not a function\n");
  } else if (!v7_is_number(msecsv)) {
    printf("msecs is not a double\n");
  } else {
    v7_val_t *cb;

    /*
     * Allocate and own the callback value. It will be disowned and freed in a
     * platform-dependent function later: see `sj_timer_callback()`,
     * `posix_timer_callback()`
     */
    cb = (v7_val_t *) malloc(sizeof(*cb));
    *cb = v7_arg(v7, 0);
    v7_own(v7, cb);

    msecs = v7_to_number(msecsv);

    sj_set_timeout(msecs, cb);
  }

  return V7_OK;
}

void sj_timers_api_setup(struct v7 *v7) {
  v7_set_method(v7, v7_get_global(v7), "setTimeout", global_set_timeout);
}
