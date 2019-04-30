/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#include <wpa2_enterprise.h>
#endif

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "mgos_gpio.h"
#include "mgos_hal.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_wifi.h"
#include "mgos_wifi_hal.h"
#include "lwip/dns.h"

static uint8_t s_cur_mode = NULL_MODE;

void wifi_changed_cb(System_Event_t *evt) {
  struct mgos_wifi_dev_event_info dei = {0};
#ifdef RTOS_SDK
  switch (evt->event_id) {
#else
  switch (evt->event) {
#endif
    case EVENT_STAMODE_DISCONNECTED:
      dei.ev = MGOS_WIFI_EV_STA_DISCONNECTED;
      dei.sta_disconnected.reason = evt->event_info.disconnected.reason;
      break;
    case EVENT_STAMODE_CONNECTED:
      dei.ev = MGOS_WIFI_EV_STA_CONNECTED;
      memcpy(dei.sta_connected.bssid, evt->event_info.connected.bssid, 6);
      dei.sta_connected.channel = evt->event_info.connected.channel;
      break;
    case EVENT_STAMODE_GOT_IP:
      dei.ev = MGOS_WIFI_EV_STA_IP_ACQUIRED;
      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
      dei.ev = MGOS_WIFI_EV_AP_STA_CONNECTED;
      memcpy(dei.ap_sta_connected.mac, evt->event_info.sta_connected.mac,
             sizeof(dei.ap_sta_connected.mac));
      break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
      dei.ev = MGOS_WIFI_EV_AP_STA_DISCONNECTED;
      memcpy(dei.ap_sta_disconnected.mac, evt->event_info.sta_disconnected.mac,
             sizeof(dei.ap_sta_disconnected.mac));
      break;
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
    case EVENT_STAMODE_AUTHMODE_CHANGE:
    case EVENT_STAMODE_DHCP_TIMEOUT:
#ifdef RTOS_SDK
    case EVENT_STAMODE_SCAN_DONE:
#endif
    case EVENT_MAX: {
      break;
    }
  }

  if (dei.ev != 0) {
    mgos_wifi_dev_event_cb(&dei);
  }
}

static bool mgos_wifi_set_mode(uint8_t mode) {
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

  s_cur_mode = mode;

  if (mode == STATION_MODE) {
    /*
     * Turn off modem sleep.
     * There is just no end to misery with it on.
     * https://github.com/espressif/ESP8266_NONOS_SDK/issues/119 is particularly
     * bad, but even without it there are regular disconnections reported by
     * multiple people (and observed by us), sometimes with device never coming
     * back (disconnect event getting lost).
     */
    wifi_set_sleep_type(NONE_SLEEP_T);
  } else {
    /* When AP is active, modem sleep is not active anyway. */
  }

  return true;
}

static bool mgos_wifi_add_mode(uint8_t mode) {
  if (s_cur_mode == mode || s_cur_mode == STATIONAP_MODE) {
    return true;
  }

  if ((s_cur_mode == SOFTAP_MODE && mode == STATION_MODE) ||
      (s_cur_mode == STATION_MODE && mode == SOFTAP_MODE)) {
    mode = STATIONAP_MODE;
  }

  return mgos_wifi_set_mode(mode);
}

static bool mgos_wifi_remove_mode(uint8_t mode) {
  if ((mode == STATION_MODE && s_cur_mode == SOFTAP_MODE) ||
      (mode == SOFTAP_MODE && s_cur_mode == STATION_MODE)) {
    /* Nothing to do. */
    return true;
  }
  if (mode == STATIONAP_MODE ||
      (mode == STATION_MODE && s_cur_mode == STATION_MODE) ||
      (mode == SOFTAP_MODE && s_cur_mode == SOFTAP_MODE)) {
    mode = NULL_MODE;
  } else if (mode == STATION_MODE) {
    mode = SOFTAP_MODE;
  } else {
    mode = STATION_MODE;
  }
  return mgos_wifi_set_mode(mode);
}

bool mgos_wifi_dev_sta_setup(const struct mgos_config_wifi_sta *cfg) {
  struct station_config sta_cfg;
  memset(&sta_cfg, 0, sizeof(sta_cfg));

  if (!cfg->enable) {
    return mgos_wifi_remove_mode(STATION_MODE);
  }

  if (!mgos_wifi_add_mode(STATION_MODE)) return false;

  wifi_station_disconnect();

  sta_cfg.bssid_set = 0;
  strncpy((char *) sta_cfg.ssid, cfg->ssid, sizeof(sta_cfg.ssid));

  if (!mgos_conf_str_empty(cfg->ip) && !mgos_conf_str_empty(cfg->netmask)) {
    struct ip_info info;
    memset(&info, 0, sizeof(info));
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    if (!mgos_conf_str_empty(cfg->gw)) info.gw.addr = ipaddr_addr(cfg->gw);
    wifi_station_dhcpc_stop();
    if (!wifi_set_ip_info(STATION_IF, &info)) {
      LOG(LL_ERROR, ("WiFi STA: Failed to set IP config"));
      return false;
    }
    LOG(LL_INFO, ("WiFi STA IP: %s/%s gw %s", cfg->ip, cfg->netmask,
                  (cfg->gw ? cfg->gw : "")));
  }

  if (mgos_conf_str_empty(cfg->user) /* Not using EAP */ &&
      !mgos_conf_str_empty(cfg->pass)) {
    strncpy((char *) sta_cfg.password, cfg->pass, sizeof(sta_cfg.password));
  }

  if (!wifi_station_set_config_current(&sta_cfg)) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set config"));
    return false;
  }

  wifi_station_set_auto_connect(0);
  wifi_station_set_reconnect_policy(0); /* We manage reconnect ourselves */

  if (!mgos_conf_str_empty(cfg->cert) || !mgos_conf_str_empty(cfg->user)) {
    /* WPA-enterprise mode */
    static char *s_ca_cert_pem = NULL, *s_cert_pem = NULL, *s_key_pem = NULL;

    wifi_station_set_enterprise_username((u8 *) cfg->user, strlen(cfg->user));

    if (!mgos_conf_str_empty(cfg->anon_identity)) {
      wifi_station_set_enterprise_identity((unsigned char *) cfg->anon_identity,
                                           strlen(cfg->anon_identity));
    } else {
      /* By default, username is used. */
      wifi_station_set_enterprise_identity((unsigned char *) cfg->user,
                                           strlen(cfg->user));
    }

    if (!mgos_conf_str_empty(cfg->pass)) {
      wifi_station_set_enterprise_password((u8 *) cfg->pass, strlen(cfg->pass));
    } else {
      wifi_station_clear_enterprise_password();
    }

    if (!mgos_conf_str_empty(cfg->ca_cert)) {
      free(s_ca_cert_pem);
      size_t len;
      s_ca_cert_pem = cs_read_file(cfg->ca_cert, &len);
      if (s_ca_cert_pem == NULL) {
        LOG(LL_ERROR, ("Failed to read %s", cfg->ca_cert));
        return false;
      }
      wifi_station_set_enterprise_ca_cert((u8 *) s_ca_cert_pem, (int) len);
    } else {
      wifi_station_clear_enterprise_ca_cert();
    }

    if (!mgos_conf_str_empty(cfg->cert) && !mgos_conf_str_empty(cfg->key)) {
      free(s_cert_pem);
      free(s_key_pem);
      size_t cert_len, key_len;
      s_cert_pem = cs_read_file(cfg->cert, &cert_len);
      if (s_cert_pem == NULL) {
        LOG(LL_ERROR, ("Failed to read %s", cfg->cert));
        return false;
      }
      s_key_pem = cs_read_file(cfg->key, &key_len);
      if (s_key_pem == NULL) {
        LOG(LL_ERROR, ("Failed to read %s", cfg->key));
        return false;
      }
      wifi_station_set_enterprise_cert_key(
          (u8 *) s_cert_pem, (int) cert_len, (u8 *) s_key_pem, (int) key_len,
          NULL /* private_key_passwd */, 0 /* private_key_passwd_len */);
    }

    wifi_station_clear_enterprise_new_password();
    wifi_station_set_enterprise_disable_time_check(true /* disable */);
    wifi_station_set_wpa2_enterprise_auth(true /* enable */);
  } else {
    wifi_station_set_wpa2_enterprise_auth(false /* enable */);
  }

  const char *host_name =
      cfg->dhcp_hostname ? cfg->dhcp_hostname : mgos_sys_config_get_device_id();
  if (host_name != NULL && !wifi_station_set_hostname((char *) host_name)) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set host name"));
    return false;
  }

  return true;
}

bool mgos_wifi_dev_ap_setup(const struct mgos_config_wifi_ap *cfg) {
  struct softap_config ap_cfg;
  memset(&ap_cfg, 0, sizeof(ap_cfg));

  if (!cfg->enable) {
    return mgos_wifi_remove_mode(SOFTAP_MODE);
  }

  if (!mgos_wifi_add_mode(SOFTAP_MODE)) return false;

  strncpy((char *) ap_cfg.ssid, cfg->ssid, sizeof(ap_cfg.ssid));
  mgos_expand_mac_address_placeholders((char *) ap_cfg.ssid);
  if (!mgos_conf_str_empty(cfg->pass)) {
    strncpy((char *) ap_cfg.password, cfg->pass, sizeof(ap_cfg.password));
    ap_cfg.authmode = AUTH_WPA2_PSK;
  } else {
    ap_cfg.authmode = AUTH_OPEN;
  }
  ap_cfg.channel = cfg->channel;
  ap_cfg.ssid_hidden = (cfg->hidden != 0);
  ap_cfg.max_connection = cfg->max_connections;
  ap_cfg.beacon_interval = 100; /* ms */
  LOG(LL_ERROR, ("WiFi AP: SSID %s, channel %d", ap_cfg.ssid, ap_cfg.channel));

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

bool mgos_wifi_dev_sta_connect(void) {
  return wifi_station_connect();
}

bool mgos_wifi_dev_sta_disconnect(void) {
  return wifi_station_disconnect();
}

bool mgos_wifi_dev_get_ip_info(int if_instance,
                               struct mgos_net_ip_info *ip_info) {
  struct ip_info info;
  if (!wifi_get_ip_info((if_instance == 0 ? STATION_IF : SOFTAP_IF), &info) ||
      info.ip.addr == 0) {
    return false;
  }
  ip_info->ip.sin_addr.s_addr = info.ip.addr;
  ip_info->netmask.sin_addr.s_addr = info.netmask.addr;
  ip_info->gw.sin_addr.s_addr = info.gw.addr;
  return true;
}

int mgos_wifi_sta_get_rssi(void) {
  int rssi = wifi_station_get_rssi();
  return (rssi < 0 ? rssi : 0);
}

void wifi_scan_done(void *arg, STATUS status) {
  if (status != OK) {
    mgos_wifi_dev_scan_cb(-1, NULL);
    return;
  }
  STAILQ_HEAD(, bss_info) *info = arg;
  struct mgos_wifi_scan_result *res = NULL;
  struct bss_info *p;
  int n = 0;
  STAILQ_FOREACH(p, info, next) n++;
  res = calloc(n, sizeof(*res));
  if (n > 0 && res == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    mgos_wifi_dev_scan_cb(-1, NULL);
    return;
  }
  struct mgos_wifi_scan_result *r = res;
  STAILQ_FOREACH(p, info, next) {
    strncpy(r->ssid, (const char *) p->ssid, sizeof(p->ssid));
    memcpy(r->bssid, p->bssid, sizeof(r->bssid));
    r->ssid[sizeof(r->ssid) - 1] = '\0';
    r->channel = p->channel;
    r->rssi = p->rssi;
    switch (p->authmode) {
      case AUTH_OPEN:
        r->auth_mode = MGOS_WIFI_AUTH_MODE_OPEN;
        break;
      case AUTH_WEP:
        r->auth_mode = MGOS_WIFI_AUTH_MODE_WEP;
        break;
      case AUTH_WPA_PSK:
        r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA_PSK;
        break;
      case AUTH_WPA2_PSK:
        r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA2_PSK;
        break;
      case AUTH_WPA_WPA2_PSK:
        r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA_WPA2_PSK;
        break;
      case AUTH_MAX:
        break;
    }
    r++;
  }
  mgos_wifi_dev_scan_cb(n, res);
}

bool mgos_wifi_dev_start_scan(void) {
  /* Scanning requires station. If in AP-only mode, switch to AP+STA. */
  if (wifi_get_opmode() == SOFTAP_MODE) {
    wifi_set_opmode_current(STATIONAP_MODE);
  }
  return wifi_station_scan(NULL, wifi_scan_done);
}

void mgos_wifi_dev_init(void) {
  wifi_set_opmode_current(NULL_MODE);
  wifi_set_event_handler_cb(wifi_changed_cb);
}

void mgos_wifi_dev_deinit(void) {
  wifi_set_opmode_current(NULL_MODE);
}

char *mgos_wifi_get_sta_default_dns(void) {
  char *dns;
  ip_addr_t dns_addr = dns_getserver(0);
  if (dns_addr.addr == 0) {
    return NULL;
  }
  if (asprintf(&dns, IPSTR, IP2STR(&dns_addr)) < 0) {
    return NULL;
  }
  return dns;
}
