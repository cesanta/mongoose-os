/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>

#include <common/platform.h>

#include "simplelink/include/simplelink.h"
#include "simplelink/include/netapp.h"
#include "simplelink/include/wlan.h"

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

#include "config.h"
#include "sys_config.h"
#include "cc3200_fs.h"
#include "cc3200_main_task.h"

struct cc3200_wifi_config {
  char *ssid;
  char *pass;
  char *ip;
  SlNetCfgIpV4Args_t static_ip;
  unsigned int status : 4;
  unsigned int reconnect : 1;
};

static struct cc3200_wifi_config s_wifi_sta_config;
static int s_current_role = -1;

static void free_wifi_config(void) {
  free(s_wifi_sta_config.ssid);
  free(s_wifi_sta_config.pass);
  free(s_wifi_sta_config.ip);
  memset(&s_wifi_sta_config, 0, sizeof(s_wifi_sta_config));
}

void invoke_wifi_on_change_cb(void *arg) {
  miot_wifi_on_change_cb((enum miot_wifi_status)(int) arg);
  if (s_current_role == ROLE_STA &&
      s_wifi_sta_config.status == MIOT_WIFI_DISCONNECTED &&
      s_wifi_sta_config.reconnect) {
    miot_wifi_connect();
  }
}

static int restart_nwp(void) {
  /* Properly close FS container if it's open for writing. */
  cc3200_fs_flush();
  /* We don't need TI's web server. */
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
  /* Without a delay in sl_Stop subsequent sl_Start gets stuck sometimes. */
  sl_Stop(10);
  s_current_role = sl_Start(NULL, NULL, NULL);
  sl_restart_cb(miot_get_mgr());
  return (s_current_role >= 0);
}

static int ensure_role_sta(void) {
  if (s_current_role == ROLE_STA) return 1;
  if (sl_WlanSetMode(ROLE_STA) != 0) return 0;
  if (!restart_nwp()) return 0;
  _u32 scan_interval = WIFI_SCAN_INTERVAL_SECONDS;
  sl_WlanPolicySet(SL_POLICY_SCAN, 1 /* enable */, (_u8 *) &scan_interval,
                   sizeof(scan_interval));
  return 1;
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  switch (e->Event) {
    case SL_WLAN_CONNECT_EVENT: {
      s_wifi_sta_config.status = MIOT_WIFI_CONNECTED;
      break;
    }
    case SL_WLAN_DISCONNECT_EVENT: {
      s_wifi_sta_config.status = MIOT_WIFI_DISCONNECTED;
      break;
    }
    default:
      return;
  }
  miot_invoke_cb(invoke_wifi_on_change_cb,
                 (void *) (intptr_t) s_wifi_sta_config.status);
}

void sl_net_app_eh(SlNetAppEvent_t *e) {
  if (e->Event == SL_NETAPP_IPV4_IPACQUIRED_EVENT) {
    SlIpV4AcquiredAsync_t *ed = &e->EventData.ipAcquiredV4;
    asprintf(&s_wifi_sta_config.ip, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(ed->ip, 3),
             SL_IPV4_BYTE(ed->ip, 2), SL_IPV4_BYTE(ed->ip, 1),
             SL_IPV4_BYTE(ed->ip, 0));
    s_wifi_sta_config.status = MIOT_WIFI_IP_ACQUIRED;
    miot_invoke_cb(invoke_wifi_on_change_cb,
                   (void *) (int) s_wifi_sta_config.status);
  } else if (e->Event == SL_NETAPP_IP_LEASED_EVENT) {
    SlIpLeasedAsync_t *ed = &e->EventData.ipLeased;
    LOG(LL_INFO,
        ("WiFi: leased %lu.%lu.%lu.%lu to %02x:%02x:%02x:%02x:%02x:%02x",
         SL_IPV4_BYTE(ed->ip_address, 3), SL_IPV4_BYTE(ed->ip_address, 2),
         SL_IPV4_BYTE(ed->ip_address, 1), SL_IPV4_BYTE(ed->ip_address, 0),
         ed->mac[0], ed->mac[1], ed->mac[2], ed->mac[3], ed->mac[4],
         ed->mac[5]));
  }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  sl_net_app_eh(e);
}

void SimpleLinkSockEventHandler(SlSockEvent_t *e) {
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *e,
                                  SlHttpServerResponse_t *resp) {
}

int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  free_wifi_config();
  s_wifi_sta_config.ssid = strdup(cfg->ssid);
  s_wifi_sta_config.pass = strdup(cfg->pass);
  memset(&s_wifi_sta_config.static_ip, 0, sizeof(s_wifi_sta_config.static_ip));
  if (cfg->ip != NULL && cfg->netmask != NULL) {
    SlNetCfgIpV4Args_t *ipcfg = &s_wifi_sta_config.static_ip;
    if (!inet_pton(AF_INET, cfg->ip, &ipcfg->ipV4) ||
        !inet_pton(AF_INET, cfg->netmask, &ipcfg->ipV4Mask) ||
        (cfg->ip != NULL &&
         !inet_pton(AF_INET, cfg->gw, &ipcfg->ipV4Gateway))) {
      return 0;
    }
  }

  return miot_wifi_connect();
}

int miot_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  int ret;
  uint8_t v;
  SlNetCfgIpV4Args_t ipcfg;
  SlNetAppDhcpServerBasicOpt_t dhcpcfg;
  char ssid[64];

  if ((ret = sl_WlanSetMode(ROLE_AP)) != 0) {
    return 0;
  }

  strncpy(ssid, cfg->ssid, sizeof(ssid));
  miot_expand_mac_address_placeholders(ssid);
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(ssid),
                        (const uint8_t *) ssid)) != 0) {
    return 0;
  }

  v = (cfg->pass != NULL && strlen(cfg->pass) > 0) ? SL_SEC_TYPE_WPA
                                                   : SL_SEC_TYPE_OPEN;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, &v)) !=
      0) {
    return 0;
  }
  if (v == SL_SEC_TYPE_WPA &&
      (ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD,
                        strlen(cfg->pass), (const uint8_t *) cfg->pass)) != 0) {
    return 0;
  }

  v = cfg->channel;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1,
                        (uint8_t *) &v)) != 0) {
    return 0;
  }

  v = cfg->hidden;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_HIDDEN_SSID, 1,
                        (uint8_t *) &v)) != 0) {
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
    return 0;
  }

  memset(&dhcpcfg, 0, sizeof(dhcpcfg));
  dhcpcfg.lease_time = 900;
  if (!inet_pton(AF_INET, cfg->dhcp_start, &dhcpcfg.ipv4_addr_start) ||
      !inet_pton(AF_INET, cfg->dhcp_end, &dhcpcfg.ipv4_addr_last) ||
      (ret = sl_NetAppSet(SL_NET_APP_DHCP_SERVER_ID,
                          NETAPP_SET_DHCP_SRV_BASIC_OPT, sizeof(dhcpcfg),
                          (uint8_t *) &dhcpcfg)) != 0) {
    return 0;
  }

  /* Turning the device off and on for the change to take effect. */
  if (!restart_nwp()) return 0;

  if ((ret = sl_NetAppStart(SL_NET_APP_DHCP_SERVER_ID)) != 0) {
    LOG(LL_ERROR, ("DHCP server failed to start: %d", ret));
  }

  sl_WlanRxStatStart();

  LOG(LL_INFO, ("AP %s configured", ssid));

  return 1;
}

int miot_wifi_connect(void) {
  int ret;
  SlSecParams_t sp;

  if (!ensure_role_sta()) return 0;

  if (s_wifi_sta_config.static_ip.ipV4 != 0) {
    ret = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE,
                       IPCONFIG_MODE_ENABLE_IPV4,
                       sizeof(s_wifi_sta_config.static_ip),
                       (unsigned char *) &s_wifi_sta_config.static_ip);
  } else {
    _u8 val = 1;
    ret = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,
                       IPCONFIG_MODE_ENABLE_IPV4, sizeof(val), &val);
  }
  if (ret != 0) return 0;

  /* Turning the device off and on for the role change to take effect. */
  if (!restart_nwp()) return 0;

  sp.Key = (_i8 *) s_wifi_sta_config.pass;
  sp.KeyLen = strlen(s_wifi_sta_config.pass);
  sp.Type = sp.KeyLen ? SL_SEC_TYPE_WPA : SL_SEC_TYPE_OPEN;

  ret = sl_WlanConnect((const _i8 *) s_wifi_sta_config.ssid,
                       strlen(s_wifi_sta_config.ssid), 0, &sp, 0);
  if (ret != 0) return 0;

  sl_WlanRxStatStart();

  LOG(LL_INFO, ("Connecting to %s", s_wifi_sta_config.ssid));

  s_wifi_sta_config.reconnect = 1;

  return 1;
}

int miot_wifi_disconnect(void) {
  free_wifi_config();
  return (sl_WlanDisconnect() == 0);
}

enum miot_wifi_status miot_wifi_get_status(void) {
  return (enum miot_wifi_status) s_wifi_sta_config.status;
}

char *miot_wifi_get_status_str(void) {
  const char *st = NULL;
  switch (s_wifi_sta_config.status) {
    case MIOT_WIFI_DISCONNECTED:
      st = "disconnected";
      break;
    case MIOT_WIFI_CONNECTED:
      st = "connected";
      break;
    case MIOT_WIFI_IP_ACQUIRED:
      st = "got ip";
      break;
  }
  if (st != NULL) return strdup(st);
  return NULL;
}

char *miot_wifi_get_connected_ssid(void) {
  switch (s_wifi_sta_config.status) {
    case MIOT_WIFI_DISCONNECTED:
      break;
    case MIOT_WIFI_CONNECTED:
    case MIOT_WIFI_IP_ACQUIRED:
      return strdup(s_wifi_sta_config.ssid);
  }
  return NULL;
}

char *miot_wifi_get_sta_ip(void) {
  if (s_wifi_sta_config.ip == NULL) return NULL;
  return strdup(s_wifi_sta_config.ip);
}

char *miot_wifi_get_ap_ip(void) {
  /* TODO(rojer?) : implement if applicable */
  return NULL;
}

void miot_wifi_scan(miot_wifi_scan_cb_t cb, void *arg) {
  const char *ssids[21];
  const char **res = NULL;
  int i, n;
  Sl_WlanNetworkEntry_t info[20];

  if (!ensure_role_sta()) goto out;

  n = sl_WlanGetNetworkList(0, 20, info);
  if (n < 0) goto out;
  for (i = 0; i < n; i++) {
    ssids[i] = (char *) info[i].ssid;
  }
  ssids[i] = NULL;
  res = ssids;
out:
  cb(res, arg);
}

void miot_wifi_hal_init(void) {
}
