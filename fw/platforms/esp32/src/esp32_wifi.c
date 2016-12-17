/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <string.h>

#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "apps/dhcpserver.h"

#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

static void invoke_wifi_on_change_cb(void *arg) {
  miot_wifi_on_change_cb((enum miot_wifi_status)(int) arg);
}
static const char *s_sta_state = NULL;

esp_err_t wifi_event_handler(system_event_t *event) {
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
          ("WiFi STA: disconnected, reason %d", info->disconnected.reason));
      mg_ev = MIOT_WIFI_DISCONNECTED;
      /* We assume connection is being retried. */
      s_sta_state = "connecting";
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      s_sta_state = "associated";
      mg_ev = MIOT_WIFI_CONNECTED;
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      /*
       * This event is forwarded to us from system handler, don't pass it on.
       * https://github.com/espressif/esp-idf/issues/161
       */
      mg_ev = MIOT_WIFI_IP_ACQUIRED;
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
    miot_invoke_cb(invoke_wifi_on_change_cb, (void *) mg_ev);
  }

  return (pass_to_system ? esp_event_send(event) : ESP_OK);
}

static esp_err_t miot_wifi_set_mode(wifi_mode_t mode) {
  esp_err_t r;

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
    return r;
  }

  r = esp_wifi_set_mode(mode);
  if (r == ESP_ERR_WIFI_NOT_INIT) {
    wifi_init_config_t icfg = {.event_handler = wifi_event_handler};
    r = esp_wifi_init(&icfg);
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("Failed to init WiFi: %d", r));
      return false;
    }
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    r = esp_wifi_set_mode(mode);
  }

  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set WiFi mode %d: %d", mode, r));
    return r;
  }

  return ESP_OK;
}

static esp_err_t miot_wifi_add_mode(wifi_mode_t mode) {
  esp_err_t r;
  wifi_mode_t cur_mode = WIFI_MODE_NULL;
  r = esp_wifi_get_mode(&cur_mode);
  /* If WIFI is not initialized yet, set_mode will do it. */
  if (r != ESP_OK && r != ESP_ERR_WIFI_NOT_INIT) {
    return r;
  }

  if (cur_mode == mode || cur_mode == WIFI_MODE_APSTA) {
    return ESP_OK;
  }

  if ((cur_mode == WIFI_MODE_AP && mode == WIFI_MODE_STA) ||
      (cur_mode == WIFI_MODE_STA && mode == WIFI_MODE_AP)) {
    mode = WIFI_MODE_APSTA;
  }

  return miot_wifi_set_mode(mode);
}

static esp_err_t miot_wifi_remove_mode(wifi_mode_t mode) {
  esp_err_t r;
  wifi_mode_t cur_mode;
  r = esp_wifi_get_mode(&cur_mode);
  if (r == ESP_ERR_WIFI_NOT_INIT) {
    /* Not initialized at all? Ok then. */
    return ESP_OK;
  }
  if ((mode == WIFI_MODE_STA && cur_mode == WIFI_MODE_AP) ||
      (mode == WIFI_MODE_AP && cur_mode == WIFI_MODE_STA)) {
    /* Nothing to do. */
    return ESP_OK;
  }
  if (mode == WIFI_MODE_APSTA ||
      (mode == WIFI_MODE_STA && cur_mode == WIFI_MODE_STA) ||
      (mode == WIFI_MODE_AP && cur_mode == WIFI_MODE_AP)) {
    mode = WIFI_MODE_NULL;
  } else if (mode == WIFI_MODE_STA) {
    mode = WIFI_MODE_AP;
  } else {
    mode = WIFI_MODE_STA;
  }
  /* As a result we will always remain in STA-only or AP-only mode. */
  return miot_wifi_set_mode(mode);
}

int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  esp_err_t r;
  wifi_config_t wcfg;
  memset(&wcfg, 0, sizeof(wcfg));
  wifi_sta_config_t *stacfg = &wcfg.sta;

  char *err_msg = NULL;
  if (!miot_wifi_validate_sta_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi STA: %s", err_msg));
    free(err_msg);
    return false;
  }

  if (!cfg->enable) {
    return (miot_wifi_remove_mode(WIFI_MODE_STA) == ESP_OK);
  }

  r = miot_wifi_add_mode(WIFI_MODE_STA);
  if (r != ESP_OK) return false;

  strncpy(stacfg->ssid, cfg->ssid, sizeof(stacfg->ssid));
  if (cfg->pass != NULL) {
    strncpy(stacfg->password, cfg->pass, sizeof(stacfg->password));
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
      return false;
    }
    LOG(LL_INFO, ("WiFi STA IP: %s/%s gw %s", cfg->ip, cfg->netmask,
                  (cfg->gw ? cfg->gw : "")));
  } else {
    tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
  }

  r = esp_wifi_set_config(WIFI_IF_STA, &wcfg);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Failed to set STA config: %d", r));
    return false;
  }

  r = esp_wifi_connect();
  if (r == ESP_ERR_WIFI_NOT_START) {
    r = esp_wifi_start();
    if (r != ESP_OK) {
      LOG(LL_ERROR, ("Failed to start WiFi: %d", r));
      return false;
    }
    r = esp_wifi_connect();
  }

  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi STA: Connect failed: %d", r));
    return false;
  }

  LOG(LL_INFO, ("WiFi STA: Connecting to %s", stacfg->ssid));

  return true;
}

int miot_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  esp_err_t r;
  wifi_config_t wcfg;
  memset(&wcfg, 0, sizeof(wcfg));
  wifi_ap_config_t *apcfg = &wcfg.ap;

  char *err_msg = NULL;
  if (!miot_wifi_validate_ap_cfg(cfg, &err_msg)) {
    LOG(LL_ERROR, ("WiFi AP: %s", err_msg));
    free(err_msg);
    return false;
  }

  if (!cfg->enable) {
    return (miot_wifi_remove_mode(WIFI_MODE_AP) == ESP_OK);
  }

  r = miot_wifi_add_mode(WIFI_MODE_AP);
  if (r != ESP_OK) return false;

  strncpy(apcfg->ssid, cfg->ssid, sizeof(apcfg->ssid));
  miot_expand_mac_address_placeholders(apcfg->ssid);
  if (cfg->pass != NULL) {
    strncpy(apcfg->password, cfg->pass, sizeof(apcfg->password));
    apcfg->authmode = WIFI_AUTH_WPA2_PSK;
  } else {
    apcfg->authmode = WIFI_AUTH_OPEN;
  }
  apcfg->channel = cfg->channel;
  apcfg->ssid_hidden = (cfg->hidden != 0);
  apcfg->max_connection = cfg->max_connections;
  apcfg->beacon_interval = 100; /* ms */

  r = esp_wifi_set_config(WIFI_IF_AP, &wcfg);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi AP: Failed to set config: %d", r));
    return false;
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
      return false;
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
      return false;
    }
  }
  r = tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("WiFi AP: Failed to start DHCP server: %d", r));
    return false;
  }
  LOG(LL_INFO,
      ("WiFi AP IP: %s/%s gw %s, DHCP range %s - %s", cfg->ip, cfg->netmask,
       (cfg->gw ? cfg->gw : "(none)"), cfg->dhcp_start, cfg->dhcp_end));

  /* There is no way to tell if AP is running already. */
  esp_wifi_start();

  LOG(LL_INFO, ("WiFi AP: SSID %s, channel %d", apcfg->ssid, apcfg->channel));

  return true;
}

static char *miot_wifi_get_ip(tcpip_adapter_if_t if_no) {
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

char *miot_wifi_get_status_str(void) {
  return (s_sta_state != NULL ? strdup(s_sta_state) : NULL);
}

char *miot_wifi_get_connected_ssid(void) {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
    return NULL;
  }
  return strdup((char *) ap_info.ssid);
}

char *miot_wifi_get_ap_ip(void) {
  return miot_wifi_get_ip(TCPIP_ADAPTER_IF_AP);
}

char *miot_wifi_get_sta_ip(void) {
  return miot_wifi_get_ip(TCPIP_ADAPTER_IF_STA);
}

bool miot_wifi_set_config(const struct sys_config_wifi *cfg) {
  bool result = false;
  if (cfg->ap.enable && !cfg->sta.enable) {
    result = miot_wifi_setup_ap(&cfg->ap);
  } else if (cfg->ap.enable && cfg->sta.enable && cfg->ap.keep_enabled) {
    result = (miot_wifi_set_mode(WIFI_MODE_APSTA) == ESP_OK &&
              miot_wifi_setup_ap(&cfg->ap) && miot_wifi_setup_sta(&cfg->sta));
  } else if (cfg->sta.enable) {
    result = miot_wifi_setup_sta(&cfg->sta);
  } else {
    result = (miot_wifi_set_mode(WIFI_MODE_NULL) == ESP_OK);
  }
  return result;
}

void miot_wifi_hal_init(void) {
}
