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

bool miot_wifi_set_config(const struct sys_config_wifi *cfg);

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

  return (miot_wifi_set_config(&cfg->wifi) ? MIOT_INIT_OK
                                           : MIOT_INIT_CONFIG_WIFI_INIT_FAILED);
}
