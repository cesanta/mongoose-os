#include "fw/src/miot_init.h"

#include "fw/src/miot_app.h"
#include "fw/src/miot_clubby.h"
#include "fw/src/miot_console.h"
#include "fw/src/miot_dns_sd.h"
#include "fw/src/miot_mdns.h"
#include "fw/src/miot_mqtt.h"
#include "fw/src/miot_service_config.h"
#include "fw/src/miot_service_filesystem.h"
#include "fw/src/miot_service_vars.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_updater_clubby.h"
#include "fw/src/miot_updater_http.h"
#include "fw/src/miot_wifi.h"

enum miot_init_result mg_init(void) {
  enum miot_init_result r = miot_sys_config_init();
  if (r != MIOT_INIT_OK) return r;
  miot_wifi_init();

#if MG_ENABLE_MDNS
  r = miot_mdns_init(); /* Before dns_sd init */
  if (r != MIOT_INIT_OK) return r;
#endif
#if MG_ENABLE_DNS_SD
  r = miot_dns_sd_init(); /* Before clubby init */
  if (r != MIOT_INIT_OK) return r;
#endif

#if MG_ENABLE_CLUBBY
  r = miot_clubby_init();
  if (r != MIOT_INIT_OK) return r;
#if MG_ENABLE_UPDATER_CLUBBY
  miot_updater_clubby_init();
#endif
#if MG_ENABLE_CONFIG_SERVICE
  miot_service_config_init();
  miot_service_vars_init();
#endif
#if MG_ENABLE_FILESYSTEM_SERVICE
  miot_service_filesystem_init();
#endif
#endif

  miot_console_init(); /* After clubby_init */

  r = miot_sys_config_init_platform(get_cfg());
  if (r != MIOT_INIT_OK) return r;

  r = miot_sys_config_init_http(&get_cfg()->http);
  if (r != MIOT_INIT_OK) return r;

#if MG_ENABLE_UPDATER
  miot_updater_http_init(); /* After HTTP init */
#endif

#if MG_ENABLE_MQTT
  r = miot_mqtt_global_init();
  if (r != MIOT_INIT_OK) return r;
#endif

  if (miot_app_init() != MIOT_APP_INIT_SUCCESS) {
    return MIOT_INIT_APP_INIT_FAILED;
  }

  return MIOT_INIT_OK;
}

enum miot_app_init_result miot_app_init(void) __attribute__((weak));
enum miot_app_init_result miot_app_init(void) {
  return MIOT_APP_INIT_SUCCESS;
}
