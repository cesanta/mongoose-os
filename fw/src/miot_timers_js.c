/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_timers_js.h"

#include <stdlib.h>

#include "fw/src/miot_common.h"
#include "fw/src/miot_v7_ext.h"

#if MG_ENABLE_JS
MG_PRIVATE enum v7_err miot_set_interval_or_timeout(struct v7 *v7,
                                                    v7_val_t *res, int repeat) {
  v7_val_t msecsv = v7_arg(v7, 1);
  int msecs;
  (void) res;

  if (!v7_is_callable(v7, v7_arg(v7, 0))) {
    printf("cb is not a function\n");
  } else if (!v7_is_number(msecsv)) {
    printf("msecs is not a number\n");
  } else {
    v7_val_t cb = v7_arg(v7, 0);
    msecs = v7_get_double(v7, msecsv);
    *res = v7_mk_number(v7, miot_set_js_timer(msecs, repeat, v7, cb));
  }

  return V7_OK;
}

MG_PRIVATE enum v7_err global_setTimeout(struct v7 *v7, v7_val_t *res) {
  return miot_set_interval_or_timeout(v7, res, 0);
}

MG_PRIVATE enum v7_err global_setInterval(struct v7 *v7, v7_val_t *res) {
  return miot_set_interval_or_timeout(v7, res, 1);
}

MG_PRIVATE enum v7_err global_clearTimeoutOrInterval(struct v7 *v7,
                                                     v7_val_t *res) {
  (void) res;
  if (v7_is_number(v7_arg(v7, 0))) {
    miot_clear_timer(v7_get_double(v7, v7_arg(v7, 0)));
  }
  return V7_OK;
}

void miot_timers_api_setup(struct v7 *v7) {
  v7_set_method(v7, v7_get_global(v7), "setTimeout", global_setTimeout);
  v7_set_method(v7, v7_get_global(v7), "setInterval", global_setInterval);
  v7_set_method(v7, v7_get_global(v7), "clearTimeout",
                global_clearTimeoutOrInterval);
  v7_set_method(v7, v7_get_global(v7), "clearInterval",
                global_clearTimeoutOrInterval);
}
#endif /* MG_ENABLE_JS */
