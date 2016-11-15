/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "mongoose/mongoose.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_wifi.h"
#include "fw/src/miot_sys_config.h"
#include "common/cs_file.h"
#include "common/cs_dbg.h"
#include "fw/platforms/esp8266/user/esp_fs.h"
#include "fw/platforms/esp8266/user/esp_gpio.h"

static int do_wifi(const struct sys_config *cfg) {
  int result = 1;
  int gpio = cfg->wifi.ap.trigger_on_gpio;
  int trigger_ap = 0;

  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);
  wifi_station_disconnect();

  if (gpio >= 0) {
    miot_gpio_set_mode(gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP);
    trigger_ap = (miot_gpio_read(gpio) == GPIO_LEVEL_LOW);
  }

  if (trigger_ap || (cfg->wifi.ap.enable && !cfg->wifi.sta.enable)) {
    LOG(LL_INFO, ("WiFi mode: AP%s", (trigger_ap ? " (triggered" : "")));
    wifi_set_opmode_current(SOFTAP_MODE);
    result = miot_wifi_setup_ap(&cfg->wifi.ap);
  } else if (cfg->wifi.ap.enable && cfg->wifi.sta.enable &&
             cfg->wifi.ap.keep_enabled) {
    LOG(LL_INFO, ("WiFi mode: AP+STA"));
    wifi_set_opmode_current(STATIONAP_MODE);
    result = miot_wifi_setup_ap(&cfg->wifi.ap);
    result = result && miot_wifi_setup_sta(&cfg->wifi.sta);
  } else if (cfg->wifi.sta.enable) {
    LOG(LL_INFO, ("WiFi mode: STA"));
    wifi_set_opmode_current(STATION_MODE);
    result = miot_wifi_setup_sta(&cfg->wifi.sta);
  } else {
    LOG(LL_INFO, ("WiFi mode: disabled"));
    miot_wifi_disconnect();
  }

  return result;
}

void device_get_mac_address(uint8_t mac[6]) {
  wifi_get_macaddr(SOFTAP_IF, mac);
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  /* Negative values mean "disable". */
  if (cfg->debug.stdout_uart > 2) {
    return MIOT_INIT_CONFIG_INVALID_STDOUT_UART;
  }
  if (cfg->debug.stderr_uart > 2) {
    return MIOT_INIT_CONFIG_INVALID_STDERR_UART;
  }

  return (do_wifi(cfg) ? MIOT_INIT_OK : MIOT_INIT_CONFIG_WIFI_INIT_FAILED);
}
