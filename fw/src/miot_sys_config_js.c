#include "fw/src/miot_sys_config_js.h"

#if MG_ENABLE_JS

#include "v7/v7.h"

#include "fw/src/miot_config_js.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"

enum v7_err Sys_conf_save(struct v7 *v7, v7_val_t *res) {
  enum v7_err ret = V7_OK;
  char *msg = NULL;
  if (save_cfg(get_cfg(), &msg)) {
    miot_system_restart(0);
    *res = v7_mk_boolean(v7, 1);
  } else {
    ret = v7_throwf(v7, "Errro saving config: %s", (msg ? msg : ""));
    free(msg);
  }
  return ret;
}

enum miot_init_result miot_sys_config_js_init(struct v7 *v7) {
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  v7_val_t conf = miot_conf_mk_proxy(v7, sys_config_schema(), get_cfg(),
                                     false /* read_only */, Sys_conf_save);
  v7_def(v7, sys, "conf", ~0, V7_DESC_ENUMERABLE(0), conf);
  v7_val_t ro_vars =
      miot_conf_mk_proxy(v7, sys_ro_vars_schema(), (void *) get_ro_vars(),
                         true /* read_only */, NULL);
  v7_def(v7, sys, "ro_vars", ~0, V7_DESC_ENUMERABLE(0), ro_vars);
  return MIOT_INIT_OK;
}

#endif /* MG_ENABLE_JS */
