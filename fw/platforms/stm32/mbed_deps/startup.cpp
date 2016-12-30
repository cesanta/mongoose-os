/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "stm32_spiffs.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_wifi.h"

int main() {
  mgos_stm32_init_spiffs_init();
  mongoose_init();
  mgos_init();

  if (get_cfg()->wifi.sta.enable) {
    mgos_wifi_setup_sta((const sys_config_wifi_sta *) &get_cfg()->wifi.sta);
  } else if (get_cfg()->wifi.ap.enable) {
    mgos_wifi_setup_ap((const sys_config_wifi_ap *) &get_cfg()->wifi.ap);
  }

  while (1) {
    mongoose_poll(0);
  }

  return 0;
}
