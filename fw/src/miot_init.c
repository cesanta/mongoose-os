#include "fw/src/miot_init.h"

#include "fw/src/miot_app.h"
#include "fw/src/miot_atca.h"
#include "fw/src/miot_console.h"
#include "fw/src/miot_dns_sd.h"
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_i2c.h"
#include "fw/src/miot_mdns.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_mqtt.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_service_config.h"
#include "fw/src/miot_service_filesystem.h"
#include "fw/src/miot_service_vars.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_updater_rpc.h"
#include "fw/src/miot_updater_http.h"
#include "fw/src/miot_wifi.h"

enum miot_init_result miot_init(void) {
  enum miot_init_result r;

  r = miot_gpio_init();
  if (r != MIOT_INIT_OK) return r;

  r = miot_sys_config_init();
  if (r != MIOT_INIT_OK) return r;

#if MIOT_ENABLE_WIFI
  r = miot_wifi_init();
  if (r != MIOT_INIT_OK) return r;
#endif

#if MIOT_ENABLE_MDNS
  r = miot_mdns_init(); /* Before dns_sd init */
  if (r != MIOT_INIT_OK) return r;
#endif
#if MIOT_ENABLE_DNS_SD
  r = miot_dns_sd_init(); /* Before miot_rpc_init */
  if (r != MIOT_INIT_OK) return r;
#endif

  /* Before miot_sys_config_init_http */
  r = miot_sys_config_init_platform(get_cfg());
  if (r != MIOT_INIT_OK) return r;

#if MIOT_ENABLE_HTTP_SERVER
  /* Before miot_rpc_init */
  r = miot_sys_config_init_http(&get_cfg()->http, &get_cfg()->device);
  if (r != MIOT_INIT_OK) return r;
#endif

#if MIOT_ENABLE_RPC
  r = miot_rpc_init(); /* After miot_sys_config_init_http */
  if (r != MIOT_INIT_OK) return r;
#if MIOT_ENABLE_UPDATER_RPC
  miot_updater_rpc_init();
#endif
#if MIOT_ENABLE_CONFIG_SERVICE
  miot_service_config_init();
  miot_service_vars_init();
#endif
#if MIOT_ENABLE_FILESYSTEM_SERVICE
  miot_service_filesystem_init();
#endif
#endif

#if MIOT_ENABLE_CONSOLE
  miot_console_init(); /* After miot_rpc_init */
#endif

#if MIOT_ENABLE_I2C
  r = miot_i2c_init();
  if (r != MIOT_INIT_OK) return r;
#endif

#if MIOT_ENABLE_ATCA
  r = miot_atca_init(); /* Requires I2C */
  if (r != MIOT_INIT_OK) return r;
#if MIOT_ENABLE_RPC && MIOT_ENABLE_ATCA_SERVICE
  r = miot_atca_service_init(); /* Requires RPC */
  if (r != MIOT_INIT_OK) return r;
#endif
#endif

#if MIOT_ENABLE_UPDATER
  miot_updater_http_init(); /* After HTTP init */
#endif

#if MIOT_ENABLE_MQTT
  r = miot_mqtt_global_init();
  if (r != MIOT_INIT_OK) return r;
#endif

  if (miot_app_init() != MIOT_APP_INIT_SUCCESS) {
    return MIOT_INIT_APP_INIT_FAILED;
  }

  LOG(LL_INFO,
      ("Init done, RAM: %d free, %d min free", (int) miot_get_free_heap_size(),
       (int) miot_get_min_free_heap_size()));
  miot_set_enable_min_heap_free_reporting(true);

  return MIOT_INIT_OK;
}

enum miot_app_init_result miot_app_init(void) __attribute__((weak));
enum miot_app_init_result miot_app_init(void) {
  return MIOT_APP_INIT_SUCCESS;
}

void miot_app_preinit(void) __attribute__((weak));
void miot_app_preinit(void) {
}
