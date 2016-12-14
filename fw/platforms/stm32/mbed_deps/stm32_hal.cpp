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

SimpleLinkInterface *wifi;

void miot_wdt_feed() {
  /* TODO(alex): implement */
}

void mongoose_schedule_poll() {
  /*
   * Mongoose is pulled in main(), so no need to do something special here
   * TODO(alashkin): it it possible to optimize here - change wait() period in main loop
   * and make mongoose pool a bit faster
   */
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  /* Do nothing here */
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

void miot_wifi_hal_init() {
  /* using pointer and new to prevent breakaway */
  wifi = new SimpleLinkInterface(PG_10, PG_11, SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS);
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
}

void miot_system_restart(int exit_code) {
  /* TODO(alex): implement */
  (void) exit_code;
}

void device_get_mac_address(uint8_t mac[6]) {
  mbed_mac_address((char*)mac);
}

void miot_wdt_set_timeout(int secs) {
  /* TODO(alex): implement */
}

int miot_gpio_write(int pin, enum gpio_level level) {
  DigitalOut gpio((PinName)pin);
  gpio.write(level);

  return 0;
}

int miot_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  /* TODO(alex): implement */
  (void) pin;
  (void) mode;
  (void) pull;
  return 0;
}
