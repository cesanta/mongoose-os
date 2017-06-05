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
  int mem1, mem2, mem3;
  mem1 = mgos_get_free_heap_size();
  struct mjs *mjs = mjs_create();
  mem2 = mgos_get_free_heap_size();
  mjs_set_ffi_resolver(mjs, mgos_dlsym);
  mjs_err_t err = mjs_exec_file(mjs, "init.js", 1, NULL);
  if (err != MJS_OK) {
    LOG(LL_ERROR, ("MJS exec error: %s\n", mjs_strerror(mjs, err)));
  }
  mem3 = mgos_get_free_heap_size();
  LOG(LL_INFO, ("mJS memory stat: before init: %d "
                "after init: %d after init.js: %d",
                mem1, mem2, mem3));
  return MGOS_APP_INIT_SUCCESS;
}
