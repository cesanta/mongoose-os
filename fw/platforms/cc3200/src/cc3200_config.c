/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/platform.h"

#include "simplelink.h"
#include "netcfg.h"

#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

#include "fw/platforms/cc3200/src/cc3200_console.h"

void device_get_mac_address(uint8_t mac[6]) {
  uint8_t mac_len = 6;
  sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &mac_len, mac);
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  if (cfg->wifi.sta.enable) {
    if (!miot_wifi_setup_sta(&cfg->wifi.sta)) {
      return MIOT_INIT_CONFIG_WIFI_INIT_FAILED;
    }
  } else if (cfg->wifi.ap.enable) {
    if (!miot_wifi_setup_ap(&cfg->wifi.ap)) {
      return MIOT_INIT_CONFIG_WIFI_INIT_FAILED;
    }
  }
  return MIOT_INIT_OK;
}
