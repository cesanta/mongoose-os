/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <malloc.h>
#include <stdio.h>

#include <common/platform.h>

#include "simplelink.h"
#include "netapp.h"
#include "wlan.h"

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "smartjs/src/sj_wifi.h"
#include "v7/v7.h"

#include "config.h"

struct cc3200_wifi_config {
  int status;
  char *ssid;
  char *pass;
  char *ip;
};

static struct cc3200_wifi_config s_wifi_sta_config;

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  enum sj_wifi_status ev = -1;
  switch (e->Event) {
    case SL_WLAN_CONNECT_EVENT: {
      s_wifi_sta_config.status = ev = SJ_WIFI_CONNECTED;
      break;
    }
    case SL_WLAN_DISCONNECT_EVENT: {
      s_wifi_sta_config.status = ev = SJ_WIFI_DISCONNECTED;
      free(s_wifi_sta_config.ip);
      s_wifi_sta_config.ip = NULL;
      break;
    }
  }
  if (ev >= 0) sj_wifi_on_change_callback(ev);
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  if (e->Event == SL_NETAPP_IPV4_IPACQUIRED_EVENT) {
    SlIpV4AcquiredAsync_t *ed = &e->EventData.ipAcquiredV4;
    asprintf(&s_wifi_sta_config.ip, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(ed->ip, 3),
             SL_IPV4_BYTE(ed->ip, 2), SL_IPV4_BYTE(ed->ip, 1),
             SL_IPV4_BYTE(ed->ip, 0));
    s_wifi_sta_config.status = SJ_WIFI_IP_ACQUIRED;
    sj_wifi_on_change_callback(SJ_WIFI_IP_ACQUIRED);
  }
}

void SimpleLinkSockEventHandler(SlSockEvent_t *e) {
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *e,
                                  SlHttpServerResponse_t *resp) {
}

int sj_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  s_wifi_sta_config.status = SJ_WIFI_DISCONNECTED;
  free(s_wifi_sta_config.ssid);
  free(s_wifi_sta_config.pass);
  free(s_wifi_sta_config.ip);
  s_wifi_sta_config.ssid = strdup(cfg->ssid);
  s_wifi_sta_config.pass = strdup(cfg->pass);
  s_wifi_sta_config.ip = NULL;

  return sj_wifi_connect();
}

int sj_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  int ret;
  uint8_t v;
  SlNetCfgIpV4Args_t ipcfg;
  SlNetAppDhcpServerBasicOpt_t dhcpcfg;

  if ((ret = sl_WlanSetMode(ROLE_AP)) != 0) {
    LOG(LL_ERROR, ("sl_WlanSetMode: %d", ret));
    return 0;
  }

  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(cfg->ssid),
                        (const uint8_t *) cfg->ssid)) != 0) {
    LOG(LL_ERROR, ("sl_WlanSet(WLAN_AP_OPT_SSID): %d", ret));
    return 0;
  }

  v = strlen(cfg->pass) > 0 ? SL_SEC_TYPE_WPA : SL_SEC_TYPE_OPEN;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, &v)) !=
      0) {
    LOG(LL_ERROR, ("sl_WlanSet(WLAN_AP_OPT_SECURITY_TYPE): %d", ret));
    return 0;
  }
  if (v == SL_SEC_TYPE_WPA &&
      (ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD,
                        strlen(cfg->pass), (const uint8_t *) cfg->pass)) != 0) {
    LOG(LL_ERROR, ("sl_WlanSet(WLAN_AP_OPT_PASSWORD): %d", ret));
    return 0;
  }

  v = cfg->channel;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1,
                        (uint8_t *) &v)) != 0) {
    LOG(LL_ERROR, ("sl_WlanSet(WLAN_AP_OPT_CHANNEL): %d", ret));
    return 0;
  }

  v = cfg->hidden;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_HIDDEN_SSID, 1,
                        (uint8_t *) &v)) != 0) {
    LOG(LL_ERROR, ("sl_WlanSet(WLAN_AP_OPT_HIDDEN_SSID): %d", ret));
    return 0;
  }

  sl_NetAppStop(SL_NET_APP_DHCP_SERVER_ID);

  memset(&ipcfg, 0, sizeof(ipcfg));
  if (!inet_pton(AF_INET, cfg->ip, &ipcfg.ipV4) ||
      !inet_pton(AF_INET, cfg->netmask, &ipcfg.ipV4Mask) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.ipV4Gateway) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.ipV4DnsServer) ||
      (ret = sl_NetCfgSet(SL_IPV4_AP_P2P_GO_STATIC_ENABLE,
                          IPCONFIG_MODE_ENABLE_IPV4, sizeof(ipcfg),
                          (uint8_t *) &ipcfg)) != 0) {
    LOG(LL_ERROR, ("sl_NetCfgSet(IPCONFIG_MODE_ENABLE_IPV4): %d", ret));
    return 0;
  }

  memset(&dhcpcfg, 0, sizeof(dhcpcfg));
  dhcpcfg.lease_time = 900;
  if (!inet_pton(AF_INET, cfg->dhcp_start, &dhcpcfg.ipv4_addr_start) ||
      !inet_pton(AF_INET, cfg->dhcp_end, &dhcpcfg.ipv4_addr_last) ||
      (ret = sl_NetAppSet(SL_NET_APP_DHCP_SERVER_ID,
                          NETAPP_SET_DHCP_SRV_BASIC_OPT, sizeof(dhcpcfg),
                          (uint8_t *) &dhcpcfg)) != 0) {
    LOG(LL_ERROR, ("sl_NetCfgSet(NETAPP_SET_DHCP_SRV_BASIC_OPT): %d", ret));
    return 0;
  }

  /* We don't need TI's web server. */
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);

  /* Turning the device off and on for the change to take effect. */
  sl_Stop(0);
  sl_Start(NULL, NULL, NULL);
  if ((ret = sl_NetAppStart(SL_NET_APP_DHCP_SERVER_ID)) != 0) {
    LOG(LL_ERROR, ("DHCP server failed to start: %d", ret));
  }

  LOG(LL_INFO, ("AP %s configured", cfg->ssid));

  return 1;
}

int sj_wifi_connect() {
  int ret;
  SlSecParams_t sp;

  if (sl_WlanSetMode(ROLE_STA) != 0) {
    return 0;
  }
  /* Turning the device off and on for the role change to take effect. */
  sl_Stop(0);
  sl_Start(NULL, NULL, NULL);

  sp.Key = (_i8 *) s_wifi_sta_config.pass;
  sp.KeyLen = strlen(s_wifi_sta_config.pass);
  sp.Type = sp.KeyLen ? SL_SEC_TYPE_WPA : SL_SEC_TYPE_OPEN;

  ret = sl_WlanConnect((const _i8 *) s_wifi_sta_config.ssid,
                       strlen(s_wifi_sta_config.ssid), 0, &sp, 0);
  if (ret != 0) {
    LOG(LL_ERROR, ("WlanConnect error: %d", ret));
    return 0;
  }

  LOG(LL_INFO, ("Connecting to %s", s_wifi_sta_config.ssid));

  return 1;
}

int sj_wifi_disconnect() {
  return (sl_WlanDisconnect() == 0);
}

enum sj_wifi_status sj_wifi_get_status() {
  return s_wifi_sta_config.status;
}

char *sj_wifi_get_status_str() {
  const char *st = NULL;
  switch (s_wifi_sta_config.status) {
    case SJ_WIFI_DISCONNECTED:
      st = "disconnected";
      break;
    case SJ_WIFI_CONNECTED:
      st = "connected";
      break;
    case SJ_WIFI_IP_ACQUIRED:
      st = "got ip";
      break;
  }
  if (st != NULL) return strdup(st);
  return NULL;
}

char *sj_wifi_get_connected_ssid() {
  switch (s_wifi_sta_config.status) {
    case SJ_WIFI_CONNECTED:
    case SJ_WIFI_IP_ACQUIRED:
      return strdup(s_wifi_sta_config.ssid);
  }
  return NULL;
}

char *sj_wifi_get_sta_ip() {
  if (s_wifi_sta_config.ip == NULL) return NULL;
  return strdup(s_wifi_sta_config.ip);
}

int sj_wifi_scan(sj_wifi_scan_cb_t cb) {
  const char *ssids[21];
  Sl_WlanNetworkEntry_t info[20];
  int i, n = sl_WlanGetNetworkList(0, 20, info);
  if (n < 0) return 0;
  for (i = 0; i < n; i++) {
    ssids[i] = (char *) info[i].ssid;
  }
  ssids[i] = NULL;
  cb(ssids);
  return 1;
}

void sj_wifi_hal_init(struct v7 *v7) {
  _u32 scan_interval = WIFI_SCAN_INTERVAL_SECONDS;
  sl_WlanPolicySet(SL_POLICY_SCAN, 1 /* enable */, (_u8 *) &scan_interval,
                   sizeof(scan_interval));
}
