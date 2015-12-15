#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "mongoose.h"
#include "sj_mongoose.h"
#include "sj_wifi.h"
#include "device_config.h"
#include "cs_file.h"
#include "cs_dbg.h"
#include "esp_uart.h"

void device_reboot(void) {
  system_restart();
}

static int do_wifi(const struct sys_config *cfg) {
  int result = 1;

  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);
  wifi_station_disconnect();

  if (cfg->wifi.ap.mode == 2 && cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATIONAP_MODE);
    result =
        sj_wifi_setup_ap(&cfg->wifi.ap) ? sj_wifi_setup_sta(&cfg->wifi.sta) : 0;
  } else if (cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATION_MODE);
    result = sj_wifi_setup_sta(&cfg->wifi.sta);
  } else if (cfg->wifi.ap.mode > 0) {
    wifi_set_opmode_current(SOFTAP_MODE);
    result = sj_wifi_setup_ap(&cfg->wifi.ap);
  } else {
    LOG(LL_WARN, ("No wifi mode specified"));
  }

  return result;
}

void device_get_mac_address(uint8_t mac[6]) {
  wifi_get_macaddr(SOFTAP_IF, mac);
}

int device_init_platform(struct sys_config *cfg) {
  /* Initialize debug first */
  uart_debug_init(0, 0);
  uart_redirect_debug(cfg->debug.mode);
  cs_log_set_level(cfg->debug.level);

  return do_wifi(cfg);
}
