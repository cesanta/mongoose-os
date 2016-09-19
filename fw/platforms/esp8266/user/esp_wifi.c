/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include "common/sha1.h"
#include <mem.h>
#include <espconn.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"

#include "fw/src/mg_hal.h"
#include "fw/src/mg_v7_ext.h"
#include "fw/platforms/esp8266/user/v7_esp.h"
#include "fw/platforms/esp8266/user/v7_esp_features.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_wifi.h"

static mg_wifi_scan_cb_t s_wifi_scan_cb;
static void *s_wifi_scan_cb_arg;

int mg_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  int res;
  struct station_config sta_cfg;
  /* If in AP-only mode, switch to station. If in STA or AP+STA, keep it. */
  if (wifi_get_opmode() == SOFTAP_MODE) {
    wifi_set_opmode_current(STATION_MODE);
  }
  wifi_station_disconnect();

  memset(&sta_cfg, 0, sizeof(sta_cfg));
  sta_cfg.bssid_set = 0;
  strncpy((char *) &sta_cfg.ssid, cfg->ssid, 32);
  strncpy((char *) &sta_cfg.password, cfg->pass, 64);

  res = wifi_station_set_config_current(&sta_cfg);
  if (!res) {
    LOG(LL_ERROR, ("Failed to set station config"));
    return 0;
  }

  if (cfg->ip != NULL && cfg->netmask != NULL) {
    struct ip_info info;
    memset(&info, 0, sizeof(info));
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    if (cfg->gw != NULL) info.gw.addr = ipaddr_addr(cfg->gw);
    wifi_station_dhcpc_stop();
    wifi_set_ip_info(STATION_IF, &info);
    LOG(LL_INFO, ("WiFi STA IP config: %s %s %s", cfg->ip, cfg->netmask,
                  (cfg->gw ? cfg->ip : "")));
  }

  LOG(LL_INFO, ("WiFi STA: Joining %s", sta_cfg.ssid));
  return wifi_station_connect();
}

int mg_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  struct softap_config ap_cfg;
  struct ip_info info;
  struct dhcps_lease dhcps;

  size_t ssid_len = (cfg->ssid != NULL ? strlen(cfg->ssid) : 0);
  size_t pass_len = (cfg->pass != NULL ? strlen(cfg->pass) : 0);

  if (ssid_len == 0) {
    LOG(LL_ERROR, ("AP SSID not set"));
    return 0;
  }
  if (ssid_len > sizeof(ap_cfg.ssid) || pass_len > sizeof(ap_cfg.password)) {
    LOG(LL_ERROR, ("AP SSID or PASS too long"));
    return 0;
  }
  if (pass_len != 0 && pass_len < 8) {
    /*
     * If we don't check pwd len here and it will be less than 8 chars
     * esp will setup _open_ wifi with name ESP_<mac address here>
     */
    LOG(LL_ERROR, ("AP password must be at least 8 chars"));
    return 0;
  }

  /* If in STA-only mode, switch to AP. If in AP or AP+STA, keep it. */
  if (wifi_get_opmode() == STATION_MODE) {
    wifi_set_opmode_current(SOFTAP_MODE);
  }

  memset(&ap_cfg, 0, sizeof(ap_cfg));
  strncpy((char *) ap_cfg.ssid, cfg->ssid, sizeof(ap_cfg.ssid));
  mg_expand_mac_address_placeholders((char *) ap_cfg.ssid);
  ap_cfg.ssid_len = ssid_len;
  if (pass_len != 0) {
    ap_cfg.authmode = AUTH_WPA2_PSK;
    strncpy((char *) ap_cfg.password, cfg->pass, sizeof(ap_cfg.password));
  } else {
    ap_cfg.authmode = AUTH_OPEN;
    ap_cfg.password[0] = '\0';
  }
  ap_cfg.channel = cfg->channel;
  ap_cfg.ssid_hidden = (cfg->hidden != 0);
  ap_cfg.max_connection = cfg->max_connections;
  ap_cfg.beacon_interval = 100; /* ms */

  LOG(LL_DEBUG, ("Setting up %s on channel %d", ap_cfg.ssid, ap_cfg.channel));
  wifi_softap_set_config_current(&ap_cfg);

  LOG(LL_DEBUG, ("Restarting DHCP server"));
  wifi_softap_dhcps_stop();

  /*
   * We have to set ESP's IP address explicitly also, GW IP has to be the
   * same. Using ap_dhcp_start as IP address for ESP
   */
  info.netmask.addr = ipaddr_addr(cfg->netmask);
  info.ip.addr = ipaddr_addr(cfg->ip);
  info.gw.addr = ipaddr_addr(cfg->gw);
  wifi_set_ip_info(SOFTAP_IF, &info);

  dhcps.enable = 1;
  dhcps.start_ip.addr = ipaddr_addr(cfg->dhcp_start);
  dhcps.end_ip.addr = ipaddr_addr(cfg->dhcp_end);
  wifi_softap_set_dhcps_lease(&dhcps);
  /* Do not offer self as a router, we're not one. */
  {
    int off = 0;
    wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &off);
  }

  wifi_softap_dhcps_start();

  wifi_get_ip_info(SOFTAP_IF, &info);
  LOG(LL_INFO, ("WiFi AP: SSID %s, channel %d, IP " IPSTR "", ap_cfg.ssid,
                ap_cfg.channel, IP2STR(&info.ip)));

  return 1;
}

int mg_wifi_connect(void) {
  return wifi_station_connect();
}

int mg_wifi_disconnect(void) {
  /* disable any AP mode */
  wifi_set_opmode_current(STATION_MODE);
  return wifi_station_disconnect();
}

enum mg_wifi_status mg_wifi_get_status(void) {
  if (wifi_station_get_connect_status() == STATION_GOT_IP) {
    return MG_WIFI_IP_ACQUIRED;
  } else {
    return MG_WIFI_DISCONNECTED;
  }
}

char *mg_wifi_get_status_str(void) {
  uint8 st = wifi_station_get_connect_status();
  const char *msg = NULL;

  switch (st) {
    case STATION_IDLE:
      msg = "idle";
      break;
    case STATION_CONNECTING:
      msg = "connecting";
      break;
    case STATION_WRONG_PASSWORD:
      msg = "bad pass";
      break;
    case STATION_NO_AP_FOUND:
      msg = "no ap";
      break;
    case STATION_CONNECT_FAIL:
      msg = "connect failed";
      break;
    case STATION_GOT_IP:
      msg = "got ip";
      break;
  }
  if (msg != NULL) return strdup(msg);
  return NULL;
}

void wifi_changed_cb(System_Event_t *evt) {
  int mg_ev = -1;
  switch (evt->event) {
    case EVENT_STAMODE_DISCONNECTED:
      mg_ev = MG_WIFI_DISCONNECTED;
      break;
    case EVENT_STAMODE_CONNECTED:
      mg_ev = MG_WIFI_CONNECTED;
      break;
    case EVENT_STAMODE_GOT_IP:
      mg_ev = MG_WIFI_IP_ACQUIRED;
      break;
  }

  if (mg_ev >= 0) mg_wifi_on_change_cb(mg_ev);
}

char *mg_wifi_get_connected_ssid(void) {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return NULL;
  return strdup((const char *) conf.ssid);
}

static char *mg_wifi_get_ip(int if_no) {
  struct ip_info info;
  char *ip;
  if (!wifi_get_ip_info(if_no, &info) || info.ip.addr == 0) return NULL;
  if (asprintf(&ip, IPSTR, IP2STR(&info.ip)) < 0) {
    return NULL;
  }
  return ip;
}

char *mg_wifi_get_ap_ip(void) {
  return mg_wifi_get_ip(1);
}

char *mg_wifi_get_sta_ip(void) {
  return mg_wifi_get_ip(0);
}

void wifi_scan_done(void *arg, STATUS status) {
  mg_wifi_scan_cb_t cb = s_wifi_scan_cb;
  void *cb_arg = s_wifi_scan_cb_arg;
  s_wifi_scan_cb = NULL;
  s_wifi_scan_cb_arg = NULL;
  if (cb == NULL) return;
  if (status != OK) {
    LOG(LL_ERROR, ("wifi scan failed: %d", status));
    cb(NULL, cb_arg);
    return;
  }
  STAILQ_HEAD(, bss_info) *info = arg;
  struct bss_info *p;
  const char **ssids;
  int n = 0;
  STAILQ_FOREACH(p, info, next) n++;
  ssids = calloc(n + 1, sizeof(*ssids));
  if (ssids == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    cb(NULL, cb_arg);
    return;
  }
  n = 0;
  STAILQ_FOREACH(p, info, next) {
    int i;
    /* Remove duplicates */
    for (i = 0; i < n; i++) {
      if (strcmp(ssids[i], (const char *) p->ssid) == 0) break;
    }
    if (i == n) ssids[n++] = (const char *) p->ssid;
  }
  cb(ssids, cb_arg);
  free(ssids);
}

void mg_wifi_scan(mg_wifi_scan_cb_t cb, void *arg) {
  /* Scanning requires station. If in AP-only mode, switch to AP+STA. */
  if (wifi_get_opmode() == SOFTAP_MODE) {
    wifi_set_opmode_current(STATIONAP_MODE);
  }
  s_wifi_scan_cb = cb;
  s_wifi_scan_cb_arg = arg;
  if (!wifi_station_scan(NULL, wifi_scan_done)) {
    cb(NULL, arg);
    s_wifi_scan_cb = NULL;
    s_wifi_scan_cb_arg = NULL;
  }
}

void mg_wifi_hal_init(void) {
  /* avoid entering AP mode on boot */
  wifi_set_opmode_current(0x1);
  wifi_set_event_handler_cb(wifi_changed_cb);
}
