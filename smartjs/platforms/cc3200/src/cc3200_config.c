/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "simplelink.h"
#include "netcfg.h"

#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_wifi.h"

#include "config.h"

void device_get_mac_address(uint8_t mac[6]) {
  uint8_t mac_len = 6;
  sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &mac_len, mac);
}

int device_init_platform(struct v7 *v7, struct sys_config *cfg) {
  (void) v7;
  if (cfg->wifi.sta.enable) {
    if (!sj_wifi_setup_sta(&cfg->wifi.sta)) {
      return 0;
    }
  } else {
    if (!sj_wifi_setup_ap(&cfg->wifi.ap)) {
      return 0;
    }
  }
  return 1; /* success */
}
