#include <stdio.h>

#include "simplelink.h"
#include "netcfg.h"

#include "device_config.h"
#include "sj_wifi.h"

#include "config.h"

void device_get_mac_address(uint8_t mac[6]) {
  uint8_t mac_len = 6;
  sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &mac_len, mac);
}

int device_init_platform(struct sys_config *cfg) {
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
