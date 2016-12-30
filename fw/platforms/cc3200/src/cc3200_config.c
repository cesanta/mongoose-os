/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/platform.h"

#include "simplelink.h"
#include "netcfg.h"

#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"

#include "fw/platforms/cc3200/src/cc3200_console.h"

void device_get_mac_address(uint8_t mac[6]) {
  uint8_t mac_len = 6;
  sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &mac_len, mac);
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
  if (cfg->wifi.sta.enable) {
    if (!mgos_wifi_setup_sta(&cfg->wifi.sta)) {
      return MGOS_INIT_CONFIG_WIFI_INIT_FAILED;
    }
  } else if (cfg->wifi.ap.enable) {
    if (!mgos_wifi_setup_ap(&cfg->wifi.ap)) {
      return MGOS_INIT_CONFIG_WIFI_INIT_FAILED;
    }
  }
  return MGOS_INIT_OK;
}
