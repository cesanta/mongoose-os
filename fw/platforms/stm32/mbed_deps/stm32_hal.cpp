/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_wifi.h"

void mongoose_schedule_poll() {
  /*
   * Mongoose is pulled in main(), so no need to do something special here
   * TODO(alashkin): it it possible to optimize here - change wait() period in
   * main loop
   * and make mongoose pool a bit faster
   */
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
  return MGOS_INIT_OK;
}

size_t mgos_get_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

size_t mgos_get_min_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

void mgos_system_restart(int exit_code) {
  (void) exit_code;
  /* TODO(alashkin): check portability */
  HAL_NVIC_SystemReset();
}

void device_get_mac_address(uint8_t mac[6]) {
  mbed_mac_address((char *) mac);
}

void mgos_wdt_feed() {
  /* TODO(alex): implement */
}

void mgos_wdt_set_timeout(int secs) {
  /* TODO(alex): implement */
}
