/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "mongoose/mongoose.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_wifi.h"
#include "smartjs/src/device_config.h"
#include "common/cs_file.h"
#include "common/cs_dbg.h"
#include "smartjs/platforms/esp8266/user/esp_fs.h"
#include "smartjs/platforms/esp8266/user/esp_gpio.h"

void device_reboot(void) {
  system_restart();
}

static int do_wifi(const struct sys_config *cfg) {
  int result = 1;
  int gpio = cfg->wifi.ap.trigger_on_gpio;
  int trigger_ap = 0;

  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);
  wifi_station_disconnect();

  if (gpio >= 0) {
    sj_gpio_set_mode(gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP);
    trigger_ap = (sj_gpio_read(gpio) == GPIO_LEVEL_LOW);
  }

  if (trigger_ap || (cfg->wifi.ap.enable && !cfg->wifi.sta.enable)) {
    LOG(LL_INFO, ("WiFi mode: AP%s", (trigger_ap ? " (triggered" : "")));
    wifi_set_opmode_current(SOFTAP_MODE);
    result = sj_wifi_setup_ap(&cfg->wifi.ap);
  } else if (cfg->wifi.ap.enable && cfg->wifi.sta.enable &&
             cfg->wifi.ap.keep_enabled) {
    LOG(LL_INFO, ("WiFi mode: AP+STA"));
    wifi_set_opmode_current(STATIONAP_MODE);
    result = sj_wifi_setup_ap(&cfg->wifi.ap);
    result = result && sj_wifi_setup_sta(&cfg->wifi.sta);
  } else if (cfg->wifi.sta.enable) {
    LOG(LL_INFO, ("WiFi mode: STA"));
    wifi_set_opmode_current(STATION_MODE);
    result = sj_wifi_setup_sta(&cfg->wifi.sta);
  } else {
    LOG(LL_INFO, ("WiFi mode: disabled"));
    sj_wifi_disconnect();
  }

  return result;
}

void device_get_mac_address(uint8_t mac[6]) {
  wifi_get_macaddr(SOFTAP_IF, mac);
}

int device_init_platform(struct v7 *v7, struct sys_config *cfg) {
  (void) v7;

  /* Negative values mean "disable". */
  if (cfg->debug.stdout_uart < 2) {
    fs_set_stdout_uart(cfg->debug.stdout_uart);
  } else {
    return 0;
  }
  if (cfg->debug.stderr_uart < 2) {
    fs_set_stderr_uart(cfg->debug.stderr_uart);
  } else {
    return 0;
  }

  cs_log_set_level(cfg->debug.level);

  return do_wifi(cfg);
}
