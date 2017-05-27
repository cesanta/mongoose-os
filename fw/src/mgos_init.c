/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos.h"

enum mgos_init_result mgos_init(void) {
  enum mgos_init_result r;

  mgos_uptime_init();

  r = mgos_timers_init();
  if (r != MGOS_INIT_OK) return r;

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
#if MGOS_ENABLE_SNTP
  r = mgos_sntp_init();
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

#if MGOS_ENABLE_SPI
  r = mgos_spi_init();
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

#if MGOS_ENABLE_ARDUINO_API
  if (mgos_arduino_init() != MGOS_INIT_OK) {
    return MGOS_INIT_APP_INIT_FAILED;
  }
#endif

  if (!mgos_deps_init()) {
    return MGOS_INIT_DEPS_FAILED;
  }

  if (mgos_app_init() != MGOS_APP_INIT_SUCCESS) {
    return MGOS_INIT_APP_INIT_FAILED;
  }

  LOG(LL_INFO, ("Init done, RAM: %d total, %d free, %d min free",
                (int) mgos_get_heap_size(), (int) mgos_get_free_heap_size(),
                (int) mgos_get_min_free_heap_size()));
  mgos_set_enable_min_heap_free_reporting(true);

  return MGOS_INIT_OK;
}

void mgos_app_preinit(void) __attribute__((weak));
void mgos_app_preinit(void) {
}

bool mgos_deps_init(void) __attribute__((weak));
bool mgos_deps_init(void) {
  return true;
}

enum mgos_app_init_result mgos_app_init(void) __attribute__((weak));
enum mgos_app_init_result mgos_app_init(void) {
  return MGOS_APP_INIT_SUCCESS;
}
