/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#undef GPIO_MODE_INPUT
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_wifi.h"
#include "fw/src/miot_gpio.h"

void mongoose_schedule_poll() {
  /* TODO(alex): implement */
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  /* TODO(alex): implement */
  return MIOT_INIT_APP_INIT_FAILED;
}

size_t miot_get_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

size_t miot_get_min_free_heap_size() {
  /* TODO(alex): implement */
  return 0;
}

void miot_wifi_hal_init() {
  /* TODO(alex): implement */
}

void miot_system_restart(int exit_code) {
  /* TODO(alex): implement */
  (void) exit_code;
}

void device_get_mac_address(uint8_t mac[6]) {
  /* TODO(alex): implement */
  (void) mac;
}

void miot_wdt_set_timeout(int secs) {
  /* TODO(alex): implement */
  (void) secs;
}

int miot_gpio_write(int pin, enum gpio_level level) {
  /* TODO(alex): implement */
  (void) pin;
  (void) level;
  return 0;
}

int miot_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  /* TODO(alex): implement */
  (void) pin;
  (void) mode;
  (void) pull;
  return 0;
}
