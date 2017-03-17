#include <stdio.h>

#include "common/platform.h"
#include "common/cs_file.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_dlsym.h"
#include "mjs.h"

enum mgos_app_init_result mgos_app_init(void) {
  /* Initialize JavaScript engine */
  struct mjs *mjs = mjs_create();
  mjs_set_ffi_resolver(mjs, mgos_dlsym);
  mjs_err_t err = mjs_exec_file(mjs, "init.js", NULL);
  if (err != MJS_OK) {
    LOG(LL_ERROR, ("MJS exec error: %s\n", mjs_strerror(mjs, err)));
  }

  return MGOS_APP_INIT_SUCCESS;
}
