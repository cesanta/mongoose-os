/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "apps/dhcpserver.h"
#include "lwip/ip_addr.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"

static void invoke_wifi_on_change_cb(void *arg) {
  mgos_wifi_on_change_cb((enum mgos_wifi_status)(int) arg);
}

SemaphoreHandle_t s_wifi_mux = NULL;
static const char *s_sta_state = NULL;
static bool s_sta_should_connect = false;
static wifi_mode_t s_cur_mode = WIFI_MODE_NULL;

static void esp32_wifi_lock() {
  while (!xSemaphoreTakeRecursive(s_wifi_mux, 10)) {
  }
}

static void esp32_wifi_unlock() {
  while (!xSemaphoreGiveRecursive(s_wifi_mux)) {
  }
}

/* Note: cannot acquire wifi lock in this handler because it gets syncronously
 * invoked from wifi task during mode changes. */
esp_err_t esp32_wifi_ev(system_event_t *event) {
  int mg_ev = -1;
  bool pass_to_system = true;
  system_event_info_t *info = &event->event_info;
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      /* We only start the station if we are connecting. */
      s_sta_state = "connecting";
    case SYSTEM_EVENT_STA_STOP:
      s_sta_state = NULL;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      LOG(LL_INFO,
          ("WiFi STA: disconnected, reason %d%s", info->disconnected.reason,
           (s_sta_should_connect ? "; reconnecting" : "")));
      mg_ev = MGOS_WIFI_DISCONNECTED;
      if (s_sta_should_connect) {
        s_sta_state = "connecting";
        esp_wifi_connect();
      } else {
        s_sta_state = "idle";
      }
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      s_sta_state = "associated";
      mg_ev = MGOS_WIFI_CONNECTED;
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      /*
       * This event is forwarded to us from system handler, don't pass it on.
       * https://github.com/espressif/esp-idf/issues/161
       */
      mg_ev = MGOS_WIFI_IP_ACQUIRED;
      s_sta_state = "got ip";
      pass_to_system = false;
      break;
    case SYSTEM_EVENT_AP_STACONNECTED: {
      const uint8_t *mac = event->event_info.sta_connected.mac;
      LOG(LL_INFO, ("WiFi AP: station %02X%02X%02X%02X%02X%02X (aid %d) %s",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                    info->sta_connected.aid, "connected"));
      break;
    }
    case SYSTEM_EVENT_AP_STADISCONNECTED: {
      const uint8_t *mac = event->event_info.sta_disconnected.mac;
      LOG(LL_INFO, ("WiFi AP: station %02X%02X%02X%02X%02X%02X (aid %d) %s",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                    info->sta_disconnected.aid, "disconnected"));
      break;
    }
    default:
      LOG(LL_INFO, ("WiFi event: %d", event->event_id));
  }

  if (mg_ev >= 0) {
    mgos_invoke_cb(invoke_wifi_on_change_cb, (void *) mg_ev,
                   false /* from_isr */);
  }

  return (pass_to_system ? esp_event_send(event) : ESP_OK);
}

static esp_err_t mgos_wifi_set_mode(wifi_mode_t mode) {
  esp_err_t r;

  esp32_wifi_lock();
  const char *mode_str = NULL;
  switch (mode) {
    case WIFI_MODE_NULL:
      mode_str = "disabled";
      break;
    case WIFI_MODE_AP:
      mode_str = "AP";
      break;
    case WIFI_MODE_STA:
      mode_str = "STA";
      break;
    case WIFI_MODE_APSTA:
      mode_str = "AP+STA";
      break;
    default:
      mode_str = "???";
  }
  LOG(LL_INFO, ("WiFi mode: %s", mode_str));

  if (mode == WIFI_MODE_NULL) {
    r = esp_wifi_stop();
    if (r == ESP_ERR_WIFI_NOT_INIT) r = ESP_OK; /* Nothing to stop. */
    if (r == ESP_OK) s_cur_mode = WIFI_MODE_NULL;
    goto out;
  }

  r = esp_wifi_set_mode(mode);
  if (r == ESP_ERR_WIFI_NOT_INIT) {
    wifi_init_config_t icfg = WIFI_INIT_CONFIG_DEFAULT();
    icfg.event_handler = esp32_wifi_ev;
    r = esp_wifi_init(&icfg);
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("Failed to init WiFi: %d", r));
      goto out;
    }
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    r = esp_wifi_set_mode(mode);
  }

  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set WiFi mode %d: %d", mode, r));
    goto out;
  }

  s_cur_mode = mode;

out:
  esp32_wifi_unlock();
  return r;
}

static esp_err_t mgos_wifi_add_mode(wifi_mode_t mode) {
  esp_err_t r = ESP_OK;

  esp32_wifi_lock();

  if (s_cur_mode == mode || s_cur_mode == WIFI_MODE_APSTA) {
    goto out;
  }

  if ((s_cur_mode == WIFI_MODE_AP && mode == WIFI_MODE_STA) ||
      (s_cur_mode == WIFI_MODE_STA && mode == WIFI_MODE_AP)) {
    mode = WIFI_MODE_APSTA;
  }

  r = mgos_wifi_set_mode(mode);

out:
  esp32_wifi_unlock();
  return r;
}

static esp_err_t mgos_wifi_remove_mode(wifi_mode_t mode) {
  esp_err_t r = ESP_OK;

  esp32_wifi_lock();

  if ((mode == WIFI_MODE_STA && s_cur_mode == WIFI_MODE_AP) ||
      (mode == WIFI_MODE_AP && s_cur_mode == WIFI_MODE_STA)) {
    /* Nothing to do. */
    goto out;
  }
  if (mode == WIFI_MODE_APSTA ||
      (mode == WIFI_MODE_STA && s_cur_mode == WIFI_MODE_STA) ||
      (mode == WIFI_MODE_AP && s_cur_mode == WIFI_MODE_AP)) {
    mode = WIFI_MODE_NULL;
  } else if (mode == WIFI_MODE_STA) {
    mode = WIFI_MODE_AP;
  } else {
    mode = WIFI_MODE_STA;
  }
  /* As a result we will always remain in STA-only or AP-only mode. */
  r = mgos_wifi_set_mode(mode);

out:
  esp32_wifi_unlock();
  return r;
}

static esp_err_t wifi_sta_set_host_name(const struct sys_config_wifi_sta *cfg) {
  esp_err_t r = ESP_OK;
  char *host_name =
      cfg->dhcp_hostname ? cfg->dhcp_hostname : get_cfg()->device.id;
  if (host_name != NULL) {
    r = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, host_name);
  }
  return r;
}

int mgos_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  bool result = false;
  esp_err_t r;
  wifi_config_t wcfg;
  memset(&wcfg, 0, sizeof(wcfg));
  wifi_sta_config_t *stacfg = &wcfg.sta;

  esp32_wifi_lock();

  char *err_msg = NULL;
  if (!mgos_wifi_validate_sta_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi STA: %s", err_msg));
    free(err_msg);
    goto out;
  }

  if (!cfg->enable) {
    s_sta_should_connect = false;
    result = (mgos_wifi_remove_mode(WIFI_MODE_STA) == ESP_OK);
    goto out;
  }

  r = mgos_wifi_add_mode(WIFI_MODE_STA);
  if (r != ESP_OK) goto out;

  strncpy((char *) stacfg->ssid, cfg->ssid, sizeof(stacfg->ssid));
  if (cfg->pass != NULL) {
    strncpy((char *) stacfg->password, cfg->pass, sizeof(stacfg->password));
  }

  if (cfg->ip != NULL && cfg->netmask != NULL) {
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    if (cfg->gw != NULL) info.gw.addr = ipaddr_addr(cfg->gw);
    r = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &info);
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("Failed to set WiFi STA IP config: %d", r));
      goto out;
    }
    LOG(LL_INFO, ("WiFi STA IP: %s/%s gw %s", cfg->ip, cfg->netmask,
                  (cfg->gw ? cfg->gw : "")));
  } else {
    tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
  }

  r = esp_wifi_set_config(WIFI_IF_STA, &wcfg);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set STA config: %d", r));
    goto out;
  }

  s_sta_should_connect = true;

  esp_err_t host_r = wifi_sta_set_host_name(cfg);
  if (host_r != ESP_OK && host_r != ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY) {
    LOG(LL_ERROR, ("WiFi STA: Failed to set host name"));
    goto out;
  }

  r = esp_wifi_connect();
  if (r == ESP_ERR_WIFI_NOT_STARTED) {
    r = esp_wifi_start();

    if (host_r != ESP_OK) {
      host_r = wifi_sta_set_host_name(cfg);
      if (host_r != ESP_OK) {
        LOG(LL_ERROR, ("WiFi STA: Failed to set host name"));
        goto out;
      }
    }

    if (r != ESP_OK) {
      LOG(LL_ERROR, ("Failed to start WiFi: %d", r));
      goto out;
    }
    r = esp_wifi_connect();
  }

  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi STA: Connect failed: %d", r));
    goto out;
  }

  LOG(LL_INFO, ("WiFi STA: Connecting to %s", stacfg->ssid));

  result = true;

out:
  esp32_wifi_unlock();
  return result;
}

int mgos_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  bool result = false;
  esp_err_t r;
  wifi_config_t wcfg;
  memset(&wcfg, 0, sizeof(wcfg));
  wifi_ap_config_t *apcfg = &wcfg.ap;

  esp32_wifi_lock();

  char *err_msg = NULL;
  if (!mgos_wifi_validate_ap_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi AP: %s", err_msg));
    free(err_msg);
    goto out;
  }

  if (!cfg->enable) {
    result = (mgos_wifi_remove_mode(WIFI_MODE_AP) == ESP_OK);
    goto out;
  }

  r = mgos_wifi_add_mode(WIFI_MODE_AP);
  if (r != ESP_OK) goto out;

  strncpy((char *) apcfg->ssid, cfg->ssid, sizeof(apcfg->ssid));
  mgos_expand_mac_address_placeholders((char *) apcfg->ssid);
  if (cfg->pass != NULL) {
    strncpy((char *) apcfg->password, cfg->pass, sizeof(apcfg->password));
    apcfg->authmode = WIFI_AUTH_WPA2_PSK;
  } else {
    apcfg->authmode = WIFI_AUTH_OPEN;
  }
  apcfg->channel = cfg->channel;
  apcfg->ssid_hidden = (cfg->hidden != 0);
  apcfg->max_connection = cfg->max_connections;
  apcfg->beacon_interval = 100; /* ms */
  LOG(LL_ERROR, ("WiFi AP: SSID %s, channel %d", apcfg->ssid, apcfg->channel));

  r = esp_wifi_set_config(WIFI_IF_AP, &wcfg);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi AP: Failed to set config: %d", r));
    goto out;
  }

  tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
  {
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    info.ip.addr = ipaddr_addr(cfg->ip);
    info.netmask.addr = ipaddr_addr(cfg->netmask);
    if (cfg->gw != NULL) info.gw.addr = ipaddr_addr(cfg->gw);
    r = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info);
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("WiFi AP: Failed to set IP config: %d", r));
      goto out;
    }
  }
  {
    dhcps_lease_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.enable = true;
    opt.start_ip.addr = ipaddr_addr(cfg->dhcp_start);
    opt.end_ip.addr = ipaddr_addr(cfg->dhcp_end);
    r = tcpip_adapter_dhcps_option(TCPIP_ADAPTER_OP_SET, REQUESTED_IP_ADDRESS,
                                   &opt, sizeof(opt));
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("WiFi AP: Failed to set DHCP config: %d", r));
      goto out;
    }
  }
  r = tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi AP: Failed to start DHCP server: %d", r));
    goto out;
  }
  LOG(LL_INFO,
      ("WiFi AP IP: %s/%s gw %s, DHCP range %s - %s", cfg->ip, cfg->netmask,
       (cfg->gw ? cfg->gw : "(none)"), cfg->dhcp_start, cfg->dhcp_end));

  /* There is no way to tell if AP is running already. */
  esp_wifi_start();

  LOG(LL_INFO, ("WiFi AP: SSID %s, channel %d", apcfg->ssid, apcfg->channel));

  result = true;

out:
  esp32_wifi_unlock();
  return result;
}

int mgos_wifi_disconnect(void) {
  esp32_wifi_lock();
  s_sta_should_connect = false;
  esp_wifi_disconnect();
  esp32_wifi_unlock();
  return true;
}

static char *mgos_wifi_get_ip(tcpip_adapter_if_t if_no) {
  tcpip_adapter_ip_info_t info;
  char *ip;
  if ((tcpip_adapter_get_ip_info(if_no, &info) != ESP_OK) ||
      info.ip.addr == 0) {
    return NULL;
  }
  if (asprintf(&ip, IPSTR, IP2STR(&info.ip)) < 0) {
    return NULL;
  }
  return ip;
}

char *mgos_wifi_get_status_str(void) {
  esp32_wifi_lock();
  char *s = (s_sta_state != NULL ? strdup(s_sta_state) : NULL);
  esp32_wifi_unlock();
  return s;
}

char *mgos_wifi_get_connected_ssid(void) {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
    return NULL;
  }
  return strdup((char *) ap_info.ssid);
}

char *mgos_wifi_get_ap_ip(void) {
  return mgos_wifi_get_ip(TCPIP_ADAPTER_IF_AP);
}

char *mgos_wifi_get_sta_ip(void) {
  return mgos_wifi_get_ip(TCPIP_ADAPTER_IF_STA);
}

bool mgos_wifi_set_config(const struct sys_config_wifi *cfg) {
  bool result = false;
  esp32_wifi_lock();
  if (cfg->ap.enable && !cfg->sta.enable) {
    result = mgos_wifi_setup_ap(&cfg->ap);
  } else if (cfg->ap.enable && cfg->sta.enable && cfg->ap.keep_enabled) {
    result = (mgos_wifi_set_mode(WIFI_MODE_APSTA) == ESP_OK &&
              mgos_wifi_setup_ap(&cfg->ap) && mgos_wifi_setup_sta(&cfg->sta));
  } else if (cfg->sta.enable) {
    result = mgos_wifi_setup_sta(&cfg->sta);
  } else {
    result = (mgos_wifi_set_mode(WIFI_MODE_NULL) == ESP_OK);
  }
  esp32_wifi_unlock();
  return result;
}

void mgos_wifi_hal_init(void) {
  s_wifi_mux = xSemaphoreCreateRecursiveMutex();
}

char *mgos_wifi_get_sta_default_gw() {
  tcpip_adapter_ip_info_t info;
  char *ip;
  if ((tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info) != ESP_OK) ||
      info.gw.addr == 0) {
    return NULL;
  }
  if (asprintf(&ip, IPSTR, IP2STR(&info.gw)) < 0) {
    return NULL;
  }
  return ip;
}

char *mgos_wifi_get_sta_default_dns() {
  char *dns;
  ip_addr_t dns_addr = dns_getserver(0);
  if (dns_addr.u_addr.ip4.addr == 0 || dns_addr.type != IPADDR_TYPE_V4) {
    return NULL;
  }
  if (asprintf(&dns, IPSTR, IP2STR(&dns_addr.u_addr.ip4)) < 0) {
    return NULL;
  }
  return dns;
}
