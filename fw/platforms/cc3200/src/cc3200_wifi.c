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
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"
#include "fw/src/mgos_wifi_hal.h"

#include "config.h"
#include "sys_config.h"
#include "fw/platforms/cc3200/src/cc3200_main_task.h"
#include "fw/platforms/cc3200/src/cc3200_vfs_dev_slfs_container.h"

struct cc3200_wifi_config {
  char *ssid;
  char *pass;
  char *user;
  char *anon_identity;
  SlIpV4AcquiredAsync_t acquired_ip;
  SlNetCfgIpV4Args_t static_ip;
};

static struct cc3200_wifi_config s_wifi_sta_config;
static int s_current_role = -1;

static void free_wifi_config(void) {
  free(s_wifi_sta_config.ssid);
  free(s_wifi_sta_config.pass);
  free(s_wifi_sta_config.user);
  free(s_wifi_sta_config.anon_identity);
  memset(&s_wifi_sta_config, 0, sizeof(s_wifi_sta_config));
}

static bool restart_nwp(void) {
  /*
   * Properly close FS container if it's open for writing.
   * Suspend FS I/O while NWP is being restarted.
   */
  mgos_lock();
  cc3200_vfs_dev_slfs_container_flush_all();
  /* We don't need TI's web server. */
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
  /* Without a delay in sl_Stop subsequent sl_Start gets stuck sometimes. */
  sl_Stop(10);
  s_current_role = sl_Start(NULL, NULL, NULL);
  mgos_unlock();
  sl_restart_cb(mgos_get_mgr());
  return (s_current_role >= 0);
}

static bool ensure_role_sta(void) {
  if (s_current_role == ROLE_STA) return true;
  if (sl_WlanSetMode(ROLE_STA) != 0) return false;
  if (!restart_nwp()) return false;
  _u32 scan_interval = WIFI_SCAN_INTERVAL_SECONDS;
  sl_WlanPolicySet(SL_POLICY_SCAN, 1 /* enable */, (_u8 *) &scan_interval,
                   sizeof(scan_interval));
  return true;
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  switch (e->Event) {
    case SL_WLAN_CONNECT_EVENT: {
      mgos_wifi_dev_on_change_cb(MGOS_WIFI_CONNECTED);
      break;
    }
    case SL_WLAN_DISCONNECT_EVENT: {
      mgos_wifi_dev_on_change_cb(MGOS_WIFI_DISCONNECTED);
      break;
    }
    default:
      return;
  }
}

void sl_net_app_eh(SlNetAppEvent_t *e) {
  if (e->Event == SL_NETAPP_IPV4_IPACQUIRED_EVENT &&
      s_current_role == ROLE_STA) {
    SlIpV4AcquiredAsync_t *ed = &e->EventData.ipAcquiredV4;
    memcpy(&s_wifi_sta_config.acquired_ip, ed, sizeof(*ed));
    mgos_wifi_dev_on_change_cb(MGOS_WIFI_IP_ACQUIRED);
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

bool mgos_wifi_dev_sta_setup(const struct sys_config_wifi_sta *cfg) {
  free_wifi_config();
  s_wifi_sta_config.ssid = strdup(cfg->ssid);
  if (cfg->pass != NULL) s_wifi_sta_config.pass = strdup(cfg->pass);
  if (cfg->user != NULL) s_wifi_sta_config.user = strdup(cfg->user);
  if (cfg->anon_identity != NULL)
    s_wifi_sta_config.anon_identity = strdup(cfg->anon_identity);

  memset(&s_wifi_sta_config.static_ip, 0, sizeof(s_wifi_sta_config.static_ip));
  if (cfg->ip != NULL && cfg->netmask != NULL) {
    SlNetCfgIpV4Args_t *ipcfg = &s_wifi_sta_config.static_ip;
    if (!inet_pton(AF_INET, cfg->ip, &ipcfg->ipV4) ||
        !inet_pton(AF_INET, cfg->netmask, &ipcfg->ipV4Mask) ||
        (cfg->ip != NULL &&
         !inet_pton(AF_INET, cfg->gw, &ipcfg->ipV4Gateway))) {
      return false;
    }
  }

  return true;
}

bool mgos_wifi_dev_ap_setup(const struct sys_config_wifi_ap *cfg) {
  int ret;
  uint8_t v;
  SlNetCfgIpV4Args_t ipcfg;
  SlNetAppDhcpServerBasicOpt_t dhcpcfg;
  char ssid[64];

  if ((ret = sl_WlanSetMode(ROLE_AP)) != 0) {
    return false;
  }

  strncpy(ssid, cfg->ssid, sizeof(ssid));
  mgos_expand_mac_address_placeholders(ssid);
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(ssid),
                        (const uint8_t *) ssid)) != 0) {
    return false;
  }

  v = (cfg->pass != NULL && strlen(cfg->pass) > 0) ? SL_SEC_TYPE_WPA
                                                   : SL_SEC_TYPE_OPEN;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, &v)) !=
      0) {
    return false;
  }
  if (v == SL_SEC_TYPE_WPA &&
      (ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD,
                        strlen(cfg->pass), (const uint8_t *) cfg->pass)) != 0) {
    return false;
  }

  v = cfg->channel;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1,
                        (uint8_t *) &v)) != 0) {
    return false;
  }

  v = cfg->hidden;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_HIDDEN_SSID, 1,
                        (uint8_t *) &v)) != 0) {
    return false;
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
    return false;
  }

  memset(&dhcpcfg, 0, sizeof(dhcpcfg));
  dhcpcfg.lease_time = 900;
  if (!inet_pton(AF_INET, cfg->dhcp_start, &dhcpcfg.ipv4_addr_start) ||
      !inet_pton(AF_INET, cfg->dhcp_end, &dhcpcfg.ipv4_addr_last) ||
      (ret = sl_NetAppSet(SL_NET_APP_DHCP_SERVER_ID,
                          NETAPP_SET_DHCP_SRV_BASIC_OPT, sizeof(dhcpcfg),
                          (uint8_t *) &dhcpcfg)) != 0) {
    return false;
  }

  /* Turning the device off and on for the change to take effect. */
  if (!restart_nwp()) return false;

  if ((ret = sl_NetAppStart(SL_NET_APP_DHCP_SERVER_ID)) != 0) {
    LOG(LL_ERROR, ("DHCP server failed to start: %d", ret));
  }

  sl_WlanRxStatStart();

  LOG(LL_INFO, ("AP %s configured", ssid));

  return true;
}

bool mgos_wifi_dev_sta_connect(void) {
  int ret;
  SlSecParams_t sp;
  SlSecParamsExt_t spext;

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
  if (ret != 0) return false;

  if (!ensure_role_sta()) return false;

  memset(&sp, 0, sizeof(sp));
  memset(&spext, 0, sizeof(spext));

  if (s_wifi_sta_config.pass != NULL) {
    sp.Key = (_i8 *) s_wifi_sta_config.pass;
    sp.KeyLen = strlen(s_wifi_sta_config.pass);
  }
  if (s_wifi_sta_config.user != NULL && get_cfg()->wifi.sta.eap_method != 0) {
    /* WPA-enterprise mode */
    sp.Type = SL_SEC_TYPE_WPA_ENT;
    spext.UserLen = strlen(s_wifi_sta_config.user);
    spext.User = (_i8 *) s_wifi_sta_config.user;
    if (s_wifi_sta_config.anon_identity != NULL) {
      spext.AnonUserLen = strlen(s_wifi_sta_config.anon_identity);
      spext.AnonUser = (_i8 *) s_wifi_sta_config.anon_identity;
    }
    spext.EapMethod = get_cfg()->wifi.sta.eap_method;
    unsigned char v = 0;
    sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 19, 1, &v);
  } else {
    sp.Type = sp.KeyLen ? SL_SEC_TYPE_WPA_WPA2 : SL_SEC_TYPE_OPEN;
  }

  ret = sl_WlanConnect((const _i8 *) s_wifi_sta_config.ssid,
                       strlen(s_wifi_sta_config.ssid), 0, &sp,
                       (sp.Type == SL_SEC_TYPE_WPA_ENT ? &spext : NULL));
  if (ret != 0) return 0;

  sl_WlanRxStatStart();

  return true;
}

bool mgos_wifi_dev_disconnect(void) {
  free_wifi_config();
  return (sl_WlanDisconnect() == 0);
}

char *mgos_wifi_get_connected_ssid(void) {
  if (s_wifi_sta_config.ssid != NULL) return strdup(s_wifi_sta_config.ssid);
  return NULL;
}

static char *ip2str(uint32_t ip) {
  char *ipstr = NULL;
  asprintf(&ipstr, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(ip, 3), SL_IPV4_BYTE(ip, 2),
           SL_IPV4_BYTE(ip, 1), SL_IPV4_BYTE(ip, 0));
  return ipstr;
}

char *mgos_wifi_get_sta_ip(void) {
  if (s_wifi_sta_config.acquired_ip.ip == 0) {
    return NULL;
  }
  return ip2str(s_wifi_sta_config.acquired_ip.ip);
}

char *mgos_wifi_get_sta_default_gw() {
  if (s_wifi_sta_config.acquired_ip.gateway == 0) {
    return NULL;
  }
  return ip2str(s_wifi_sta_config.acquired_ip.gateway);
}

char *mgos_wifi_get_sta_default_dns(void) {
  if (s_wifi_sta_config.acquired_ip.dns == 0) {
    return NULL;
  }
  return ip2str(s_wifi_sta_config.acquired_ip.dns);
}

char *mgos_wifi_get_ap_ip(void) {
  /* TODO(rojer?) : implement if applicable */
  return NULL;
}

bool mgos_wifi_dev_start_scan(void) {
  bool ret = false;
  int n = -1, num_res = 0;
  struct mgos_wifi_scan_result *res = NULL;
  Sl_WlanNetworkEntry_t info[2];

  if (!ensure_role_sta()) goto out;

  while ((n = sl_WlanGetNetworkList(num_res, 2, info)) > 0) {
    int i, j;
    res = (struct mgos_wifi_scan_result *) realloc(
        res, (num_res + n) * sizeof(*res));
    if (res == NULL) {
      goto out;
    }
    for (i = 0, j = num_res; i < n; i++) {
      Sl_WlanNetworkEntry_t *e = &info[i];
      struct mgos_wifi_scan_result *r = &res[j];
      strncpy(r->ssid, (const char *) e->ssid, sizeof(r->ssid));
      memcpy(r->bssid, e->bssid, sizeof(r->bssid));
      r->ssid[sizeof(r->ssid) - 1] = '\0';
      r->channel = 0; /* n/a */
      switch (e->sec_type) {
        case SL_SCAN_SEC_TYPE_OPEN:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_OPEN;
          break;
        case SL_SCAN_SEC_TYPE_WEP:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WEP;
          break;
        case SL_SCAN_SEC_TYPE_WPA:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA_PSK;
          break;
        case SL_SCAN_SEC_TYPE_WPA2:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA2_PSK;
          break;
        default:

          continue;
      }
      r->rssi = e->rssi;
      num_res++;
      j++;
    }
  }
  ret = (n == 0); /* Reached the end of the list */

out:
  if (ret) {
    mgos_wifi_dev_scan_cb(num_res, res);
  } else {
    free(res);
  }
  return ret;
}

void mgos_wifi_dev_init(void) {
}
