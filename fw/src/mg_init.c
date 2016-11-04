#include "fw/src/mg_init.h"

#include "fw/src/mg_app.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/mg_console.h"
#include "fw/src/mg_dns_sd.h"
#include "fw/src/mg_mdns.h"
#include "fw/src/mg_mqtt.h"
#include "fw/src/mg_service_config.h"
#include "fw/src/mg_service_filesystem.h"
#include "fw/src/mg_service_vars.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_updater_clubby.h"
#include "fw/src/mg_updater_http.h"
#include "fw/src/mg_wifi.h"

enum mg_init_result mg_init(void) {
  enum mg_init_result r = mg_sys_config_init();
  if (r != MG_INIT_OK) return r;
  mg_wifi_init();

#if MG_ENABLE_MDNS
  r = mg_mdns_init(); /* Before dns_sd init */
  if (r != MG_INIT_OK) return r;
#endif
#if MG_ENABLE_DNS_SD
  r = mg_dns_sd_init(); /* Before clubby init */
  if (r != MG_INIT_OK) return r;
#endif

#if MG_ENABLE_CLUBBY
  r = mg_clubby_init();
  if (r != MG_INIT_OK) return r;
#if MG_ENABLE_UPDATER_CLUBBY
  mg_updater_clubby_init();
#endif
#if MG_ENABLE_CONFIG_SERVICE
  mg_service_config_init();
  mg_service_vars_init();
#endif
#if MG_ENABLE_FILESYSTEM_SERVICE
  mg_service_filesystem_init();
#endif
#endif

  mg_console_init(); /* After clubby_init */

  r = mg_sys_config_init_platform(get_cfg());
  if (r != MG_INIT_OK) return r;

  r = mg_sys_config_init_http(&get_cfg()->http);
  if (r != MG_INIT_OK) return r;

#if MG_ENABLE_UPDATER
  mg_updater_http_init(); /* After HTTP init */
#endif

#if MG_ENABLE_MQTT
  r = mg_mqtt_global_init();
  if (r != MG_INIT_OK) return r;
#endif

  if (mg_app_init() != MG_APP_INIT_SUCCESS) {
    return MG_INIT_APP_INIT_FAILED;
  }

  return MG_INIT_OK;
}

enum mg_app_init_result mg_app_init(void) __attribute__((weak));
enum mg_app_init_result mg_app_init(void) {
  return MG_APP_INIT_SUCCESS;
}
