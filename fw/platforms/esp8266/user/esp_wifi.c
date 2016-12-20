/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include "common/sha1.h"
#include <mem.h>
#include <espconn.h>

#include "common/cs_dbg.h"

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_v7_ext.h"
#include "fw/src/miot_wifi.h"

#include "fw/platforms/esp8266/user/v7_esp.h"

static miot_wifi_scan_cb_t s_wifi_scan_cb;
static void *s_wifi_scan_cb_arg;

void wifi_changed_cb(System_Event_t *evt) {
  int mg_ev = -1;
  switch (evt->event) {
    case EVENT_STAMODE_DISCONNECTED:
      mg_ev = MIOT_WIFI_DISCONNECTED;
      break;
    case EVENT_STAMODE_CONNECTED:
      mg_ev = MIOT_WIFI_CONNECTED;
      break;
    case EVENT_STAMODE_GOT_IP:
      mg_ev = MIOT_WIFI_IP_ACQUIRED;
      break;
  }

  if (mg_ev >= 0) miot_wifi_on_change_cb(mg_ev);
}

static bool miot_wifi_set_mode(uint8_t mode) {
  const char *mode_str = NULL;
  switch (mode) {
    case NULL_MODE:
      mode_str = "disabled";
      break;
    case SOFTAP_MODE:
      mode_str = "AP";
      break;
    case STATION_MODE:
      mode_str = "STA";
      break;
    case STATIONAP_MODE:
      mode_str = "AP+STA";
      break;
    default:
      mode_str = "???";
  }
  LOG(LL_INFO, ("WiFi mode: %s", mode_str));

  if (!wifi_set_opmode_current(mode)) {
    LOG(LL_ERROR, ("Failed to set WiFi mode %d", mode));
    return false;
  }

  return true;
}

static bool miot_wifi_add_mode(uint8_t mode) {
  uint8_t cur_mode = wifi_get_opmode();

  if (cur_mode == mode || cur_mode == STATIONAP_MODE) {
    return true;
  }

  if ((cur_mode == SOFTAP_MODE && mode == STATION_MODE) ||
      (cur_mode == STATION_MODE && mode == SOFTAP_MODE)) {
    mode = STATIONAP_MODE;
  }

  return miot_wifi_set_mode(mode);
}

static bool miot_wifi_remove_mode(uint8_t mode) {
  uint8_t cur_mode = wifi_get_opmode();

  if ((mode == STATION_MODE && cur_mode == SOFTAP_MODE) ||
      (mode == SOFTAP_MODE && cur_mode == STATION_MODE)) {
    /* Nothing to do. */
    return true;
  }
  if (mode == STATIONAP_MODE ||
      (mode == STATION_MODE && cur_mode == STATION_MODE) ||
      (mode == SOFTAP_MODE && cur_mode == SOFTAP_MODE)) {
    mode = NULL_MODE;
  } else if (mode == STATION_MODE) {
    mode = SOFTAP_MODE;
  } else {
    mode = STATION_MODE;
  }
  return miot_wifi_set_mode(mode);
}

int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  struct station_config sta_cfg;
  memset(&sta_cfg, 0, sizeof(sta_cfg));

  char *err_msg = NULL;
  if (!miot_wifi_validate_sta_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi STA: %s", err_msg));
    free(err_msg);
    return false;
  }

  if (!cfg->enable) {
    return miot_wifi_remove_mode(STATION_MODE);
  }

  if (!miot_wifi_add_mode(STATION_MODE)) return false;

  wifi_station_disconnect();

  sta_cfg.bssid_set = 0;
  strncpy((char *) sta_cfg.ssid, cfg->ssid, sizeof(sta_cfg.ssid));
  strncpy((char *) sta_cfg.password, cfg->pass, sizeof(sta_cfg.password));

  if (cfg->ip != NULL && cfg->netmask != NULL) {
    struct ip_info info;
    memset(&info, 0, sizeof(info));
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    if (cfg->gw != NULL) info.gw.addr = ipaddr_addr(cfg->gw);
    wifi_station_dhcpc_stop();
    if (!wifi_set_ip_info(STATION_IF, &info)) {
      LOG(LL_ERROR, ("WiFi STA: Failed to set IP config"));
      return false;
    }
    LOG(LL_INFO, ("WiFi STA IP: %s/%s gw %s", cfg->ip, cfg->netmask,
                  (cfg->gw ? cfg->gw : "")));
  }

  if (!wifi_station_set_config_current(&sta_cfg)) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set config"));
    return false;
  }

  if (!wifi_station_connect()) {
    LOG(LL_ERROR, ("WiFi STA: Connect failed"));
    return false;
  }

  LOG(LL_INFO, ("WiFi STA: Connecting to %s", sta_cfg.ssid));

  return true;
}

int miot_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  struct softap_config ap_cfg;
  memset(&ap_cfg, 0, sizeof(ap_cfg));

  char *err_msg = NULL;
  if (!miot_wifi_validate_ap_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi AP: %s", err_msg));
    free(err_msg);
    return false;
  }

  if (!cfg->enable) {
    return miot_wifi_remove_mode(SOFTAP_MODE);
  }

  if (!miot_wifi_add_mode(SOFTAP_MODE)) return false;

  strncpy((char *) ap_cfg.ssid, cfg->ssid, sizeof(ap_cfg.ssid));
  miot_expand_mac_address_placeholders((char *) ap_cfg.ssid);
  if (cfg->pass != NULL) {
    strncpy((char *) ap_cfg.password, cfg->pass, sizeof(ap_cfg.password));
    ap_cfg.authmode = AUTH_WPA2_PSK;
  } else {
    ap_cfg.authmode = AUTH_OPEN;
  }
  ap_cfg.channel = cfg->channel;
  ap_cfg.ssid_hidden = (cfg->hidden != 0);
  ap_cfg.max_connection = cfg->max_connections;
  ap_cfg.beacon_interval = 100; /* ms */

  if (!wifi_softap_set_config_current(&ap_cfg)) {
    LOG(LL_ERROR, ("WiFi AP: Failed to set config"));
    return false;
  }

  wifi_softap_dhcps_stop();
  {
    /*
     * We have to set ESP's IP address explicitly also, GW IP has to be the
     * same. Using ap_dhcp_start as IP address for ESP
     */
    struct ip_info info;
    memset(&info, 0, sizeof(info));
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.gw.addr = ipaddr_addr(cfg->gw);
    if (!wifi_set_ip_info(SOFTAP_IF, &info)) {
      LOG(LL_ERROR, ("WiFi AP: Failed to set IP config"));
      return false;
    }
  }
  {
    struct dhcps_lease dhcps;
    memset(&dhcps, 0, sizeof(dhcps));
    dhcps.enable = 1;
    dhcps.start_ip.addr = ipaddr_addr(cfg->dhcp_start);
    dhcps.end_ip.addr = ipaddr_addr(cfg->dhcp_end);
    if (!wifi_softap_set_dhcps_lease(&dhcps)) {
      LOG(LL_ERROR, ("WiFi AP: Failed to set DHCP config"));
      return false;
    }
    /* Do not offer self as a router, we're not one. */
    {
      int off = 0;
      wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &off);
    }
  }
  if (!wifi_softap_dhcps_start()) {
    LOG(LL_ERROR, ("WiFi AP: Failed to start DHCP server"));
    return false;
  }

  LOG(LL_INFO,
      ("WiFi AP IP: %s/%s gw %s, DHCP range %s - %s", cfg->ip, cfg->netmask,
       (cfg->gw ? cfg->gw : "(none)"), cfg->dhcp_start, cfg->dhcp_end));

  return true;
}

int miot_wifi_connect(void) {
  return wifi_station_connect();
}

int miot_wifi_disconnect(void) {
  /* disable any AP mode */
  wifi_set_opmode_current(STATION_MODE);
  return wifi_station_disconnect();
}

enum miot_wifi_status miot_wifi_get_status(void) {
  if (wifi_station_get_connect_status() == STATION_GOT_IP) {
    return MIOT_WIFI_IP_ACQUIRED;
  } else {
    return MIOT_WIFI_DISCONNECTED;
  }
}

char *miot_wifi_get_status_str(void) {
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

char *miot_wifi_get_connected_ssid(void) {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return NULL;
  return strdup((const char *) conf.ssid);
}

static char *miot_wifi_get_ip(int if_no) {
  struct ip_info info;
  char *ip;
  if (!wifi_get_ip_info(if_no, &info) || info.ip.addr == 0) return NULL;
  if (asprintf(&ip, IPSTR, IP2STR(&info.ip)) < 0) {
    return NULL;
  }
  return ip;
}

char *miot_wifi_get_ap_ip(void) {
  return miot_wifi_get_ip(1);
}

char *miot_wifi_get_sta_ip(void) {
  return miot_wifi_get_ip(0);
}

void wifi_scan_done(void *arg, STATUS status) {
  miot_wifi_scan_cb_t cb = s_wifi_scan_cb;
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

void miot_wifi_scan(miot_wifi_scan_cb_t cb, void *arg) {
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

bool miot_wifi_set_config(const struct sys_config_wifi *cfg) {
  bool result = false;
  int gpio = cfg->ap.trigger_on_gpio;
  int trigger_ap = 0;

  if (gpio >= 0) {
    miot_gpio_set_mode(gpio, MIOT_GPIO_MODE_INPUT);
    miot_gpio_set_pull(gpio, MIOT_GPIO_PULL_UP);
    trigger_ap = (miot_gpio_read(gpio) == 0);
  }

  if (trigger_ap || (cfg->ap.enable && !cfg->sta.enable)) {
    result = miot_wifi_setup_ap(&cfg->ap);
  } else if (cfg->ap.enable && cfg->sta.enable && cfg->ap.keep_enabled) {
    result = (miot_wifi_set_mode(STATIONAP_MODE) &&
              miot_wifi_setup_ap(&cfg->ap) && miot_wifi_setup_sta(&cfg->sta));
  } else if (cfg->sta.enable) {
    result = miot_wifi_setup_sta(&cfg->sta);
  } else {
    result = miot_wifi_set_mode(NULL_MODE);
  }

  return result;
}

void miot_wifi_hal_init(void) {
  wifi_set_opmode_current(NULL_MODE);
  wifi_set_event_handler_cb(wifi_changed_cb);
}
