/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos.h"

enum mgos_init_result mgos_init(void) {
  enum mgos_init_result r;

  mgos_uptime_init();

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
#if MGOS_ENABLE_SNTP
  r = mgos_sntp_init();
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_I2C
  r = mgos_i2c_init();
  if (r != MGOS_INIT_OK) return r;
#endif

#if MGOS_ENABLE_SPI
  r = mgos_spi_init();
  if (r != MGOS_INIT_OK) return r;
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

  /* Invoke all registered init_done hooks */
  mgos_hook_trigger(MGOS_HOOK_INIT_DONE, NULL);

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
