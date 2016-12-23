/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_wifi.h"

void mongoose_schedule_poll() {
  /*
   * Mongoose is pulled in main(), so no need to do something special here
   * TODO(alashkin): it it possible to optimize here - change wait() period in
   * main loop
   * and make mongoose pool a bit faster
   */
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  return MIOT_INIT_OK;
}

size_t miot_get_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

size_t miot_get_min_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

void miot_system_restart(int exit_code) {
  (void) exit_code;
  /* TODO(alashkin): check portability */
  HAL_NVIC_SystemReset();
}

void device_get_mac_address(uint8_t mac[6]) {
  mbed_mac_address((char *) mac);
}

void miot_wdt_feed() {
  /* TODO(alex): implement */
}

void miot_wdt_set_timeout(int secs) {
  /* TODO(alex): implement */
}
