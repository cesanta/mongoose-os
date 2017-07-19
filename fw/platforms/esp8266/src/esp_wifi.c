/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
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

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_net_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"
#include "fw/src/mgos_wifi_hal.h"
#include "lwip/dns.h"

static uint8_t s_cur_mode = NULL_MODE;

void wifi_changed_cb(System_Event_t *evt) {
  bool send_ev = false;
  enum mgos_net_event mg_ev;
#ifdef RTOS_SDK
  switch (evt->event_id) {
#else
  switch (evt->event) {
#endif
    case EVENT_STAMODE_DISCONNECTED:
      mg_ev = MGOS_NET_EV_DISCONNECTED;
      send_ev = true;
      break;
    case EVENT_STAMODE_CONNECTED:
      mg_ev = MGOS_NET_EV_CONNECTED;
      send_ev = true;
      break;
    case EVENT_STAMODE_GOT_IP:
      mg_ev = MGOS_NET_EV_IP_ACQUIRED;
      send_ev = true;
      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
    case EVENT_SOFTAPMODE_STADISCONNECTED:
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
    case EVENT_STAMODE_AUTHMODE_CHANGE:
    case EVENT_STAMODE_DHCP_TIMEOUT:
#ifdef RTOS_SDK
    case EVENT_STAMODE_SCAN_DONE:
#endif
    case EVENT_MAX: {
      LOG(LL_DEBUG, ("WiFi event: %d", evt->event));
      break;
    }
  }

  if (send_ev) {
    mgos_wifi_dev_on_change_cb(mg_ev);
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

bool mgos_wifi_dev_sta_setup(const struct sys_config_wifi_sta *cfg) {
  struct station_config sta_cfg;
  memset(&sta_cfg, 0, sizeof(sta_cfg));

  if (!cfg->enable) {
    return mgos_wifi_remove_mode(STATION_MODE);
  }

  if (!mgos_wifi_add_mode(STATION_MODE)) return false;

  wifi_station_disconnect();

  sta_cfg.bssid_set = 0;
  strncpy((char *) sta_cfg.ssid, cfg->ssid, sizeof(sta_cfg.ssid));

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

  if (cfg->user == NULL /* Not using EAP */ && cfg->pass != NULL) {
    strncpy((char *) sta_cfg.password, cfg->pass, sizeof(sta_cfg.password));
  }

  if (!wifi_station_set_config_current(&sta_cfg)) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set config"));
    return false;
  }

  if (cfg->cert != NULL || cfg->user != NULL) {
    /* WPA-enterprise mode */
    static char *s_ca_cert_pem = NULL, *s_cert_pem = NULL, *s_key_pem = NULL;

    wifi_station_set_enterprise_username((u8 *) cfg->user, strlen(cfg->user));

    if (cfg->anon_identity != NULL) {
      wifi_station_set_enterprise_identity((unsigned char *) cfg->anon_identity,
                                           strlen(cfg->anon_identity));
    } else {
      /* By default, username is used. */
      wifi_station_set_enterprise_identity((unsigned char *) cfg->user,
                                           strlen(cfg->user));
    }

    if (cfg->pass != NULL) {
      wifi_station_set_enterprise_password((u8 *) cfg->pass, strlen(cfg->pass));
    } else {
      wifi_station_clear_enterprise_password();
    }

    if (cfg->ca_cert != NULL) {
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

    if (cfg->cert != NULL && cfg->key != NULL) {
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

  char *host_name =
      cfg->dhcp_hostname ? cfg->dhcp_hostname : get_cfg()->device.id;
  if (host_name != NULL && !wifi_station_set_hostname(host_name)) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set host name"));
    return false;
  }

  return true;
}

bool mgos_wifi_dev_ap_setup(const struct sys_config_wifi_ap *cfg) {
  struct softap_config ap_cfg;
  memset(&ap_cfg, 0, sizeof(ap_cfg));

  if (!cfg->enable) {
    return mgos_wifi_remove_mode(SOFTAP_MODE);
  }

  if (!mgos_wifi_add_mode(SOFTAP_MODE)) return false;

  strncpy((char *) ap_cfg.ssid, cfg->ssid, sizeof(ap_cfg.ssid));
  mgos_expand_mac_address_placeholders((char *) ap_cfg.ssid);
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

char *mgos_wifi_get_connected_ssid(void) {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return NULL;
  return strdup((const char *) conf.ssid);
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
  if (res == NULL) {
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

bool mgos_wifi_set_config(const struct sys_config_wifi *cfg) {
  bool result = false;
  int gpio = cfg->ap.trigger_on_gpio;
  int trigger_ap = 0;

  if (gpio >= 0) {
    mgos_gpio_set_mode(gpio, MGOS_GPIO_MODE_INPUT);
    mgos_gpio_set_pull(gpio, MGOS_GPIO_PULL_UP);
    trigger_ap = (mgos_gpio_read(gpio) == 0);
  }

  if (trigger_ap || (cfg->ap.enable && !cfg->sta.enable)) {
    result = mgos_wifi_setup_ap(&cfg->ap);
  } else if (cfg->ap.enable && cfg->sta.enable && cfg->ap.keep_enabled) {
    result = (mgos_wifi_set_mode(STATIONAP_MODE) &&
              mgos_wifi_setup_ap(&cfg->ap) && mgos_wifi_setup_sta(&cfg->sta));
  } else if (cfg->sta.enable) {
    result = mgos_wifi_setup_sta(&cfg->sta);
  } else {
    result = mgos_wifi_set_mode(NULL_MODE);
  }

  return result;
}

void mgos_wifi_dev_init(void) {
  wifi_set_opmode_current(NULL_MODE);
  wifi_set_event_handler_cb(wifi_changed_cb);
}

char *mgos_wifi_get_sta_default_dns() {
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
