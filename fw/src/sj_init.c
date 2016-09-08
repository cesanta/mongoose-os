#include "fw/src/sj_init.h"

#include "fw/src/sj_app.h"
#include "fw/src/sj_console.h"
#include "fw/src/sj_init_clubby.h"
#include "fw/src/sj_service_vars.h"
#include "fw/src/sj_service_config.h"
#include "fw/src/sj_sys_config.h"
#include "fw/src/sj_updater_post.h"
#include "fw/src/sj_updater_clubby.h"
#include "fw/src/sj_wifi.h"

enum sj_init_result sj_init(void) {
  enum sj_init_result r = sj_sys_config_init();
  if (r != SJ_INIT_OK) return r;
  sj_wifi_init();

#ifdef SJ_ENABLE_CLUBBY
  clubby_init();
#ifdef SJ_ENABLE_UPDATER_CLUBBY
  sj_updater_clubby_init();
#endif
#ifdef SJ_ENABLE_CONFIG_SERVICE
  sj_service_vars_init();
  sj_service_config_init();
#endif
#endif

  sj_console_init(); /* After clubby_init */

  r = sj_sys_config_init_platform(get_cfg());
  if (r != SJ_INIT_OK) return r;

  r = sj_sys_config_init_http(&get_cfg()->http);
  if (r != SJ_INIT_OK) return r;

#ifdef SJ_ENABLE_UPDATER_POST
  sj_updater_post_init(); /* After HTTP init */
#endif

  if (sj_app_init() != MG_APP_INIT_SUCCESS) {
    return SJ_INIT_APP_INIT_FAILED;
  }

  return SJ_INIT_OK;
}

enum mg_app_init_result sj_app_init(void) __attribute__((weak));
enum mg_app_init_result sj_app_init(void) {
  return MG_APP_INIT_SUCCESS;
}
