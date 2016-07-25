#include "fw/src/sj_sys_config_js.h"

#ifdef SJ_ENABLE_JS

#include "v7/v7.h"

#include "fw/src/sj_config_js.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_sys_config.h"

enum v7_err Sys_conf_save(struct v7 *v7, v7_val_t *res) {
  int res_b = 0;
  if (save_cfg(get_cfg()) == 0) {
    sj_system_restart(0);
    res_b = 1;
  }
  *res = v7_mk_boolean(v7, res_b);
  return V7_OK;
}

enum sj_init_result sj_sys_config_js_init(struct v7 *v7) {
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  v7_val_t conf = sj_conf_mk_proxy(v7, sys_config_schema(), get_cfg(),
                                   false /* read_only */, Sys_conf_save);
  v7_def(v7, sys, "conf", ~0, V7_DESC_ENUMERABLE(0), conf);
  v7_val_t ro_vars =
      sj_conf_mk_proxy(v7, sys_ro_vars_schema(), (void *) get_ro_vars(),
                       true /* read_only */, NULL);
  v7_def(v7, sys, "ro_vars", ~0, V7_DESC_ENUMERABLE(0), ro_vars);
  return SJ_INIT_OK;
}

#endif /* SJ_ENABLE_JS */
