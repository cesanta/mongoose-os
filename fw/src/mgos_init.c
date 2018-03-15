/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <time.h>

#include "mgos.h"
#include "mgos_deps_internal.h"
#include "mgos_gpio_internal.h"
#include "mgos_net_internal.h"
#include "mgos_sys_config_internal.h"
#include "mgos_timers_internal.h"

enum mgos_init_result mgos_init(void) {
  enum mgos_init_result r;

  mgos_event_register_base(MGOS_EVENT_SYS, "mos");
  mgos_uptime_init();

  r = mgos_net_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_gpio_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_sys_config_init();
  if (r != MGOS_INIT_OK) return r;

#ifdef __NEWLIB__
  {
    /* initialize TZ env variable with the sys.tz_spec config value */
    const char *tz_spec = mgos_sys_config_get_sys_tz_spec();
    if (tz_spec == NULL) tz_spec = "";
    setenv("TZ", tz_spec, 1);
    tzset();
  }
#else
/* TODO(rojer): TZ support for TI libc */
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

  /* Invoke all registered init_done event handlers */
  mgos_event_trigger(MGOS_EVENT_INIT_DONE, NULL);

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
