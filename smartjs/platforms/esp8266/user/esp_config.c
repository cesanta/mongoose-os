#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "mongoose.h"
#include "sj_mongoose.h"
#include "device_config.h"
#include "cs_file.h"
#include "cs_dbg.h"
#include "esp_uart.h"

void device_reboot(void) {
  system_restart();
}

static int do_station(const struct sys_config *cfg) {
  struct station_config sta_cfg;

  wifi_station_disconnect();

  if (strlen(cfg->wifi.sta.ssid) > sizeof(sta_cfg.ssid) ||
      strlen(cfg->wifi.sta.pass) > sizeof(sta_cfg.password)) {
    LOG(LL_ERROR, ("WIFI station SSID or PASS too long"));
    return 0;
  }

  memset(&sta_cfg, 0, sizeof(sta_cfg));
  strncpy((char *) sta_cfg.ssid, cfg->wifi.sta.ssid, sizeof(sta_cfg.ssid));
  strncpy((char *) sta_cfg.password, cfg->wifi.sta.pass,
          sizeof(sta_cfg.password));
  sta_cfg.bssid_set = 0;

  wifi_station_set_config_current(&sta_cfg);

  LOG(LL_DEBUG, ("Joining %s", sta_cfg.ssid));
  wifi_station_connect();

  return 1;
}

static int do_ap(const struct sys_config *cfg) {
  struct softap_config ap_cfg;
  struct ip_info info;
  struct dhcps_lease dhcps;

  memset(&ap_cfg, 0, sizeof(ap_cfg));

  int pass_len = strlen(cfg->wifi.ap.pass);
  int ssid_len = strlen(cfg->wifi.ap.ssid);

  if (ssid_len > sizeof(ap_cfg.ssid) || pass_len > sizeof(ap_cfg.password)) {
    LOG(LL_ERROR, ("AP SSID or PASS too long"));
    return 0;
  }

  if (pass_len != 0 && pass_len < 8) {
    /*
     * If we don't check pwd len here and it will be less than 8 chars
     * esp will setup _open_ wifi with name ESP_<mac address here>
     */
    LOG(LL_ERROR, ("AP password too short"));
    return 0;
  }

  strncpy((char *) ap_cfg.ssid, cfg->wifi.ap.ssid, sizeof(ap_cfg.ssid));
  strncpy((char *) ap_cfg.password, cfg->wifi.ap.pass, sizeof(ap_cfg.password));
  ap_cfg.ssid_len = ssid_len;
  if (pass_len != 0) {
    ap_cfg.authmode = AUTH_WPA2_PSK;
  }
  ap_cfg.channel = cfg->wifi.ap.channel;
  ap_cfg.ssid_hidden = (cfg->wifi.ap.hidden != 0);
  ap_cfg.max_connection = 1;
  ap_cfg.beacon_interval = 100; /* ms */

  LOG(LL_DEBUG, ("Setting up %s on channel %d", ap_cfg.ssid, ap_cfg.channel));
  wifi_softap_set_config_current(&ap_cfg);

  LOG(LL_DEBUG, ("Restarting DHCP server"));
  wifi_softap_dhcps_stop();

  /*
   * We have to set ESP's IP address explicitly also, GW IP has to be the
   * same. Using ap_dhcp_start as IP address for ESP
   */
  info.netmask.addr = ipaddr_addr(cfg->wifi.ap.dhcp_netmask);
  info.ip.addr = info.gw.addr = ipaddr_addr(cfg->wifi.ap.dhcp_start);
  wifi_set_ip_info(SOFTAP_IF, &info);

  dhcps.start_ip.addr = ipaddr_addr(cfg->wifi.ap.dhcp_start);
  dhcps.end_ip.addr = ipaddr_addr(cfg->wifi.ap.dhcp_end);
  wifi_softap_set_dhcps_lease(&dhcps);

  wifi_softap_dhcps_start();

  wifi_get_ip_info(SOFTAP_IF, &info);
  LOG(LL_INFO, ("AP address is " IPSTR "", IP2STR(&info.ip)));

  return 1;
}

static int do_wifi(const struct sys_config *cfg) {
  int result = 1;

  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);
  wifi_station_disconnect();

  if (cfg->wifi.ap.mode == 2 && cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATIONAP_MODE);
    result = do_station(cfg) ? do_ap(cfg) : 0;
  } else if (cfg->wifi.ap.mode >= 1 && !cfg->wifi.sta.enable) {
    wifi_set_opmode_current(SOFTAP_MODE);
    result = do_ap(cfg);
  } else if (cfg->wifi.ap.mode >= 1 && cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATION_MODE);
    result = do_station(cfg);
  } else {
    LOG(LL_WARN, ("No wifi mode specified"));
    return 1;
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

  if (!do_wifi(cfg)) {
    return 0;
  }

  return 1;
}
