/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#endif

#include "mongoose/mongoose.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_wifi.h"
#include "fw/src/mgos_sys_config.h"
#include "common/cs_file.h"
#include "common/cs_dbg.h"

bool mgos_wifi_set_config(const struct sys_config_wifi *cfg);

void device_get_mac_address(uint8_t mac[6]) {
  wifi_get_macaddr(SOFTAP_IF, mac);
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
  return (mgos_wifi_set_config(&cfg->wifi) ? MGOS_INIT_OK
                                           : MGOS_INIT_CONFIG_WIFI_INIT_FAILED);
}
