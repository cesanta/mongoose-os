#include "fw/src/mgos_init.h"

#include "fw/src/mgos_app.h"
#include "fw/src/mgos_atca.h"
#include "fw/src/mgos_console.h"
#include "fw/src/mgos_dns_sd.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_gpio_service.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_i2c_service.h"
#include "fw/src/mgos_mdns.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_mqtt.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_service_config.h"
#include "fw/src/mgos_service_filesystem.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_updater_rpc.h"
#include "fw/src/mgos_updater_http.h"
#include "fw/src/mgos_wifi.h"

enum mgos_init_result mgos_init(void) {
  enum mgos_init_result r;

  r = mgos_gpio_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_sys_config_init();
  if (r != MGOS_INIT_OK) return r;

#if MGOS_ENABLE_WIFI
  r = mgos_wifi_init();
  if (r != MGOS_INIT_OK) return r;
#endif

  /* Before mgos_sys_config_init_http */
  r = mgos_sys_config_init_platform(get_cfg());
  if (r != MGOS_INIT_OK) return r;

#if MGOS_ENABLE_MDNS
  r = mgos_mdns_init(); /* Before dns_sd init, after
                           mgos_sys_config_init_platform */
  if (r != MGOS_INIT_OK) return r;
#endif
#if MGOS_ENABLE_DNS_SD
  r = mgos_dns_sd_init(); /* Before mgos_rpc_init */
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_I2C
  r = mgos_i2c_init();
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_ATCA
  r = mgos_atca_init(); /* Requires I2C */
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_HTTP_SERVER
  /* Before mgos_rpc_init */
  r = mgos_sys_config_init_http(&get_cfg()->http, &get_cfg()->device);
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_RPC
  r = mgos_rpc_init(); /* After mgos_sys_config_init_http */
  if (r != MGOS_INIT_OK) return r;
#if MGOS_ENABLE_UPDATER_RPC
  mgos_updater_rpc_init();
#endif
#if MGOS_ENABLE_CONFIG_SERVICE
  mgos_service_config_init();
#endif
#if MGOS_ENABLE_FILESYSTEM_SERVICE
  mgos_service_filesystem_init();
#endif
#endif
#if MGOS_ENABLE_GPIO_SERVICE
  r = mgos_gpio_service_init();
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_CONSOLE
  mgos_console_init(); /* After mgos_rpc_init */
#endif

#if MGOS_ENABLE_ATCA && MGOS_ENABLE_RPC && MGOS_ENABLE_ATCA_SERVICE
  r = mgos_atca_service_init(); /* Requires RPC */
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_I2C && MGOS_ENABLE_RPC && MGOS_ENABLE_I2C_SERVICE
  r = mgos_i2c_service_init();
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_UPDATER
  mgos_updater_http_init(); /* After HTTP init */
#endif

#if MGOS_ENABLE_MQTT
  r = mgos_mqtt_global_init();
  if (r != MGOS_INIT_OK) return r;
#endif

  if (mgos_app_init() != MGOS_APP_INIT_SUCCESS) {
    return MGOS_INIT_APP_INIT_FAILED;
  }

  LOG(LL_INFO,
      ("Init done, RAM: %d free, %d min free", (int) mgos_get_free_heap_size(),
       (int) mgos_get_min_free_heap_size()));
  mgos_set_enable_min_heap_free_reporting(true);

  return MGOS_INIT_OK;
}

enum mgos_app_init_result mgos_app_init(void) __attribute__((weak));
enum mgos_app_init_result mgos_app_init(void) {
  return MGOS_APP_INIT_SUCCESS;
}

void mgos_app_preinit(void) __attribute__((weak));
void mgos_app_preinit(void) {
}
