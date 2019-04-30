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

#include <stdio.h>
#include <stdlib.h>

#include <common/platform.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "common/platforms/simplelink/sl_fs_slfs.h"

#include "mgos_file_utils.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_utils.h"
#include "mgos_wifi_hal.h"

#if CS_PLATFORM == CS_P_CC3200
#include "cc3200_vfs_dev_slfs_container.h"
#endif

#ifndef WIFI_SCAN_INTERVAL_SECONDS
#define WIFI_SCAN_INTERVAL_SECONDS 15
#endif

#define SL_CERT_FILE_NAME "/sys/cert/client.der"
#define SL_KEY_FILE_NAME "/sys/cert/private.key"
#define SL_CA_FILE_NAME "/sys/cert/ca.der"
#define DUMMY_TOKEN 0x12345678

/* Compatibility with older versions of SimpleLink */
#if SL_MAJOR_VERSION_NUM < 2
#define SL_NETAPP_DHCP_SERVER_ID SL_NET_APP_DHCP_SERVER_ID
#define SL_NETAPP_HTTP_SERVER_ID SL_NET_APP_HTTP_SERVER_ID

#define SL_NETAPP_EVENT_IPV4_ACQUIRED SL_NETAPP_IPV4_IPACQUIRED_EVENT
#define SL_NETAPP_EVENT_DHCPV4_LEASED SL_NETAPP_IP_LEASED_EVENT

#define SL_NETAPP_DHCP_SRV_BASIC_OPT NETAPP_SET_DHCP_SRV_BASIC_OPT

#define SL_NETCFG_IPV4_STA_ADDR_MODE SL_IPV4_STA_P2P_CL_GET_INFO
#define SL_NETCFG_IPV4_AP_ADDR_MODE SL_IPV4_AP_P2P_GO_GET_INFO

#define SL_WLAN_EVENT_CONNECT SL_WLAN_CONNECT_EVENT
#define SL_WLAN_EVENT_DISCONNECT SL_WLAN_DISCONNECT_EVENT

#define SL_WLAN_AP_OPT_SSID WLAN_AP_OPT_SSID
#define SL_WLAN_AP_OPT_CHANNEL WLAN_AP_OPT_CHANNEL
#define SL_WLAN_AP_OPT_HIDDEN_SSID WLAN_AP_OPT_HIDDEN_SSID
#define SL_WLAN_AP_OPT_SECURITY_TYPE WLAN_AP_OPT_SECURITY_TYPE
#define SL_WLAN_AP_OPT_PASSWORD WLAN_AP_OPT_PASSWORD
#define SL_WLAN_AP_OPT_MAX_STATIONS WLAN_AP_OPT_MAX_STATIONS

#define SL_WLAN_POLICY_PM SL_POLICY_PM
#define SL_WLAN_POLICY_SCAN SL_POLICY_SCAN

#define SL_WLAN_ALWAYS_ON_POLICY SL_ALWAYS_ON_POLICY

#define SL_WLAN_SEC_TYPE_OPEN SL_SEC_TYPE_OPEN
#define SL_WLAN_SEC_TYPE_WEP SL_SEC_TYPE_WEP
#define SL_WLAN_SEC_TYPE_WPA SL_SEC_TYPE_WPA
#define SL_WLAN_SEC_TYPE_WPA_WPA2 SL_SEC_TYPE_WPA_WPA2
#define SL_WLAN_SEC_TYPE_WPA_ENT SL_SEC_TYPE_WPA_ENT

#define SL_WLAN_SECURITY_TYPE_BITMAP_OPEN SL_SCAN_SEC_TYPE_OPEN
#define SL_WLAN_SECURITY_TYPE_BITMAP_WEP SL_SCAN_SEC_TYPE_WEP
#define SL_WLAN_SECURITY_TYPE_BITMAP_WPA SL_SCAN_SEC_TYPE_WPA
#define SL_WLAN_SECURITY_TYPE_BITMAP_WPA2 SL_SCAN_SEC_TYPE_WPA2

#define SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE \
  WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE

#define SlWlanNetworkEntry_t Sl_WlanNetworkEntry_t
#define SlWlanGetRxStatResponse_t SlGetRxStatResponse_t

#endif

struct sl_sta_cfg {
  char *ssid;
#if SL_MAJOR_VERSION_NUM >= 2
  SlWlanSecParams_t sp;
  SlWlanSecParamsExt_t spext;
#else
  SlSecParams_t sp;
  SlSecParamsExt_t spext;
#endif
};
static struct sl_sta_cfg s_sta_cfg;
static int s_current_role = -1;

static bool restart_nwp(SlWlanMode_e role) {
  /*
   * Properly close FS container if it's open for writing.
   * Suspend FS I/O while NWP is being restarted.
   */
  mgos_lock();
#if CS_PLATFORM == CS_P_CC3200
  cc3200_vfs_dev_slfs_container_flush_all();
#endif
  /* Enable channels 12-14 */
  const _u8 *val = "JP";
  sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
             SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE, 2, val);
  if (sl_WlanSetMode(role) != 0) return false;
  /* Without a delay in sl_Stop subsequent sl_Start gets stuck sometimes. */
  sl_Stop(10);
  s_current_role = sl_Start(NULL, NULL, NULL);
  mgos_unlock();
  /* We don't need TI's web server. */
  sl_NetAppStop(SL_NETAPP_HTTP_SERVER_ID);
  /*
   * Reportedly CC3200 NWP goes haywire when trying to do PM, so disable it.
   * TODO(rojer): Check if it still applies to CC3220.
   */
  sl_WlanPolicySet(SL_WLAN_POLICY_PM, SL_WLAN_ALWAYS_ON_POLICY, NULL, 0);
  sl_restart_cb(mgos_get_mgr());
  return (s_current_role >= 0);
}

static bool ensure_role_sta(void) {
  if (s_current_role == ROLE_STA) return true;
  if (!restart_nwp(ROLE_STA)) return false;
  _u32 scan_interval = WIFI_SCAN_INTERVAL_SECONDS;
  sl_WlanPolicySet(SL_WLAN_POLICY_SCAN, 1 /* enable */, (_u8 *) &scan_interval,
                   sizeof(scan_interval));
  return true;
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
#if SL_MAJOR_VERSION_NUM >= 2
  _u32 eid = e->Id;
#else
  _u32 eid = e->Event;
#endif
  struct mgos_wifi_dev_event_info dei;
  memset(&dei, 0, sizeof(dei));
  switch (eid) {
    case SL_WLAN_EVENT_CONNECT: {
      dei.ev = MGOS_WIFI_EV_STA_CONNECTED;
#if SL_MAJOR_VERSION_NUM >= 2
      memcpy(dei.sta_connected.bssid, e->Data.Connect.Bssid, 6);
#else
      memcpy(dei.sta_connected.bssid, e->EventData.STAandP2PModeWlanConnected.bssid, 6);
#endif
      break;
    }
    case SL_WLAN_EVENT_DISCONNECT: {
      dei.ev = MGOS_WIFI_EV_STA_DISCONNECTED;
#if SL_MAJOR_VERSION_NUM >= 2
      dei.sta_disconnected.reason = e->Data.Disconnect.ReasonCode;
#else
      dei.sta_disconnected.reason = e->EventData.STAandP2PModeDisconnected.reason_code;
#endif
      break;
    }
#if SL_MAJOR_VERSION_NUM >= 2
    case SL_WLAN_EVENT_STA_ADDED: {
      dei.ev = MGOS_WIFI_EV_AP_STA_CONNECTED;
      memcpy(dei.ap_sta_connected.mac, e->Data.STAAdded.Mac, 6);
      break;
    }
    case SL_WLAN_EVENT_STA_REMOVED: {
      dei.ev = MGOS_WIFI_EV_AP_STA_DISCONNECTED;
      memcpy(dei.ap_sta_disconnected.mac, e->Data.STARemoved.Mac, 6);
      break;
    }
#endif
    default:
      return;
  }
  if (dei.ev != 0) {
    mgos_wifi_dev_event_cb(&dei);
  }
}

void sl_net_app_eh(SlNetAppEvent_t *e) {
#if SL_MAJOR_VERSION_NUM >= 2
  _u32 eid = e->Id;
  SlNetAppEventData_u *edu = &e->Data;
#else
  _u32 eid = e->Event;
  SlNetAppEventData_u *edu = &e->EventData;
#endif
  if (eid == SL_NETAPP_EVENT_IPV4_ACQUIRED && s_current_role == ROLE_STA) {
    struct mgos_wifi_dev_event_info dei = {
      .ev = MGOS_WIFI_EV_STA_IP_ACQUIRED,
    };
    mgos_wifi_dev_event_cb(&dei);
  } else if (eid == SL_NETAPP_EVENT_DHCPV4_LEASED) {
#if SL_MAJOR_VERSION_NUM >= 2
    _u32 ip = edu->IpLeased.IpAddress;
    _u8 *mac = edu->IpLeased.Mac;
#else
    _u32 ip = edu->ipLeased.ip_address;
    _u8 *mac = edu->ipLeased.mac;
    (void) ip;
    (void) mac;
#endif
    LOG(LL_INFO,
        ("WiFi: leased %lu.%lu.%lu.%lu to %02x:%02x:%02x:%02x:%02x:%02x",
         SL_IPV4_BYTE(ip, 3), SL_IPV4_BYTE(ip, 2), SL_IPV4_BYTE(ip, 1),
         SL_IPV4_BYTE(ip, 0), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
  }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  sl_net_app_eh(e);
}

#if SL_MAJOR_VERSION_NUM >= 2
#define EAP_METHOD_NOT_SET 0
#define EAP_METHOD_INVALID ((uint32_t) -1)

static uint32_t eap_method_from_str(const char *s) {
  if (strcmp(s, "TLS") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_TLS;
  } else if (strcmp(s, "TTLS_TLS") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_TTLS_TLS;
  } else if (strcmp(s, "TTLS_MSCHAPv2") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_TTLS_MSCHAPv2;
  } else if (strcmp(s, "TTLS_PSK") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_TTLS_PSK;
  } else if (strcmp(s, "PEAP0_TLS") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_PEAP0_TLS;
  } else if (strcmp(s, "PEAP0_MSCHAPv2") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_PEAP0_MSCHAPv2;
  } else if (strcmp(s, "PEAP0_PSK") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_PEAP0_PSK;
  } else if (strcmp(s, "PEAP1_TLS") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_PEAP1_TLS;
  } else if (strcmp(s, "PEAP1_PSK") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_PEAP1_PSK;
  } else if (strcmp(s, "FAST_AUTH_PROVISIONING") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_FAST_AUTH_PROVISIONING;
  } else if (strcmp(s, "FAST_UNAUTH_PROVISIONING") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_FAST_UNAUTH_PROVISIONING;
  } else if (strcmp(s, "FAST_NO_PROVISIONING") == 0) {
    return SL_WLAN_ENT_EAP_METHOD_FAST_NO_PROVISIONING;
  } else {
    LOG(LL_ERROR, ("Invalid eap_method"));
    return EAP_METHOD_INVALID;
  }
}

static const char *eap_method_to_str(uint32_t m) {
  switch (m) {
    case SL_WLAN_ENT_EAP_METHOD_TLS:
      return "TLS";
    case SL_WLAN_ENT_EAP_METHOD_TTLS_TLS:
      return "TTLS_TLS";
    case SL_WLAN_ENT_EAP_METHOD_TTLS_MSCHAPv2:
      return "TTLS_MSCHAPv2";
    case SL_WLAN_ENT_EAP_METHOD_TTLS_PSK:
      return "TTLS_PSK";
    case SL_WLAN_ENT_EAP_METHOD_PEAP0_TLS:
      return "PEAP0_TLS";
    case SL_WLAN_ENT_EAP_METHOD_PEAP0_MSCHAPv2:
      return "PEAP0_MSCHAPv2";
    case SL_WLAN_ENT_EAP_METHOD_PEAP0_PSK:
      return "PEAP0_PSK";
    case SL_WLAN_ENT_EAP_METHOD_PEAP1_TLS:
      return "PEAP1_TLS";
    case SL_WLAN_ENT_EAP_METHOD_PEAP1_PSK:
      return "PEAP1_PSK";
    case SL_WLAN_ENT_EAP_METHOD_FAST_AUTH_PROVISIONING:
      return "FAST_AUTH_PROVISIONING";
    case SL_WLAN_ENT_EAP_METHOD_FAST_UNAUTH_PROVISIONING:
      return "FAST_UNAUTH_PROVISIONING";
    case SL_WLAN_ENT_EAP_METHOD_FAST_NO_PROVISIONING:
      return "FAST_NO_PROVISIONING";
  }
  return "INVALID";
}

static uint32_t get_eap_method(const struct mgos_config_wifi_sta *cfg) {
  /* If eap_method is specified, use it. */
  if (!mgos_conf_str_empty(cfg->eap_method)) {
    return eap_method_from_str(cfg->eap_method);
  }

  /* If full set of TLS credentials are set, it's plain TLS. */
  if (!mgos_conf_str_empty(cfg->cert) && !mgos_conf_str_empty(cfg->key) &&
      !mgos_conf_str_empty(cfg->ca_cert)) {
    return SL_WLAN_ENT_EAP_METHOD_TLS;
  }
  /*
   * If user and CA are provided but client cert is not, it's PEAP.
   * Assume v0 as most widely used. If user is set, it's MSCHAP, otherwise PSK.
   */
  if (!mgos_conf_str_empty(cfg->ca_cert) && mgos_conf_str_empty(cfg->cert)) {
    if (!mgos_conf_str_empty(cfg->user)) {
      return SL_WLAN_ENT_EAP_METHOD_PEAP0_MSCHAPv2;
    } else {
      return SL_WLAN_ENT_EAP_METHOD_PEAP0_PSK;
    }
  }
  return EAP_METHOD_NOT_SET;
}
#endif /* SL_MAJOR_VERSION_NUM >= 2 */

bool mgos_wifi_dev_sta_setup(const struct mgos_config_wifi_sta *cfg) {
  bool ret;

  if (!ensure_role_sta()) return false;

  mgos_conf_set_str(&s_sta_cfg.ssid, cfg->ssid);

  mgos_conf_set_str((char **) &s_sta_cfg.sp.Key, cfg->pass);
  s_sta_cfg.sp.KeyLen = (cfg->pass ? strlen(cfg->pass) : 0);

#if SL_MAJOR_VERSION_NUM >= 2
  s_sta_cfg.spext.EapMethod = get_eap_method(cfg);
  if (s_sta_cfg.spext.EapMethod != EAP_METHOD_NOT_SET) {
    if (s_sta_cfg.spext.EapMethod == EAP_METHOD_INVALID) return false;

    /* WPA-enterprise mode */
    s_sta_cfg.sp.Type = SL_WLAN_SEC_TYPE_WPA_ENT;
    LOG(LL_INFO, ("WPA-ENT mode, method: %s",
                  eap_method_to_str(s_sta_cfg.spext.EapMethod)));

    mgos_conf_set_str((char **) &s_sta_cfg.spext.User, cfg->user);
    s_sta_cfg.spext.UserLen = (cfg->user ? strlen(cfg->user) : 0);

    mgos_conf_set_str((char **) &s_sta_cfg.spext.AnonUser, cfg->anon_identity);
    s_sta_cfg.spext.AnonUserLen =
        (cfg->anon_identity ? strlen(cfg->anon_identity) : 0);

    uint32_t token = DUMMY_TOKEN;
    bool cert_auth_disable = cfg->eap_cert_validation_disable;
    if (cfg->ca_cert != NULL) {
      fs_slfs_set_file_flags(SL_CA_FILE_NAME, SL_FS_CREATE_VENDOR_TOKEN |
                                                  SL_FS_CREATE_NOSIGNATURE |
                                                  SL_FS_CREATE_PUBLIC_READ,
                             &token);
      ret = mgos_file_copy_if_different(cfg->ca_cert, "/slfs" SL_CA_FILE_NAME);
      fs_slfs_unset_file_flags(SL_CA_FILE_NAME);
      if (!ret) return false;
      fs_slfs_unset_file_flags(SL_CA_FILE_NAME);

      /*
       * If time is not set, connection will not work, for sure.
       * However, we may get time just as soon as we connect, so disable cert
       * auth.
       */
      SlDateTime_t dt;
      _u8 opt = SL_DEVICE_GENERAL_DATE_TIME;
      _u16 len = sizeof(dt);
      if (sl_DeviceGet(SL_DEVICE_GENERAL, &opt, &len, (_u8 *) (&dt)) == 0 &&
          dt.tm_year < 2018) {
        LOG(LL_INFO,
            ("Time is not set (%04u/%02u/%02u), disabling cert validation",
             dt.tm_year, dt.tm_mon, dt.tm_day));
        cert_auth_disable = true;
      }
    }

    if (cfg->cert &&
        !mgos_file_copy_if_different(cfg->cert, "/slfs" SL_CERT_FILE_NAME)) {
      return false;
    }
    if (cfg->key &&
        !mgos_file_copy_if_different(cfg->key, "/slfs" SL_KEY_FILE_NAME)) {
      return false;
    }

    if (cert_auth_disable) {
      LOG(LL_WARN,
          ("Warning: EAP cert auth is disabled, connection is not secure!"));
    }
    /* Despite the name, to disable cert check value needs to be 0. */
    uint8_t value = (cert_auth_disable ? 0 : 1);
    sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
               SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH, 1, &value);

    unsigned char v = 0;
    sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 19, 1, &v);
  } else
#endif /* SL_MAJOR_VERSION_NUM >= 2 */
      if (s_sta_cfg.sp.KeyLen > 0) {
    s_sta_cfg.sp.Type = SL_WLAN_SEC_TYPE_WPA_WPA2;
  } else {
    s_sta_cfg.sp.Type = SL_WLAN_SEC_TYPE_OPEN;
  }

  if (!mgos_conf_str_empty(cfg->ip) && !mgos_conf_str_empty(cfg->netmask)) {
    SlNetCfgIpV4Args_t ipcfg;
#if SL_MAJOR_VERSION_NUM >= 2
    if (!inet_pton(AF_INET, cfg->ip, &ipcfg.Ip) ||
        !inet_pton(AF_INET, cfg->netmask, &ipcfg.IpMask) ||
        (!mgos_conf_str_empty(cfg->ip) &&
         !inet_pton(AF_INET, cfg->gw, &ipcfg.IpGateway))) {
      return false;
    }
    ret = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_STATIC,
                       sizeof(ipcfg), (unsigned char *) &ipcfg);
#else
    if (!inet_pton(AF_INET, cfg->ip, &ipcfg.ipV4) ||
        !inet_pton(AF_INET, cfg->netmask, &ipcfg.ipV4Mask) ||
        (!mgos_conf_str_empty(cfg->ip) &&
         !inet_pton(AF_INET, cfg->gw, &ipcfg.ipV4Gateway))) {
      return false;
    }
    ret = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE,
                       IPCONFIG_MODE_ENABLE_IPV4, sizeof(ipcfg),
                       (unsigned char *) &ipcfg);
#endif
  } else {
#if SL_MAJOR_VERSION_NUM >= 2
    ret = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_DHCP, 0, 0);
#else
    _u8 val = 1;
    ret = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,
                       IPCONFIG_MODE_ENABLE_IPV4, sizeof(val), &val);
#endif
  }
  if (ret != 0) return false;

  return true;
}

bool mgos_wifi_dev_ap_setup(const struct mgos_config_wifi_ap *cfg) {
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
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SSID, strlen(ssid),
                        (const uint8_t *) ssid)) != 0) {
    return false;
  }

  v = mgos_conf_str_empty(cfg->pass) ? SL_WLAN_SEC_TYPE_OPEN
                                     : SL_WLAN_SEC_TYPE_WPA;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SECURITY_TYPE, 1,
                        &v)) != 0) {
    return false;
  }
  if (v == SL_WLAN_SEC_TYPE_WPA &&
      (ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_PASSWORD,
                        strlen(cfg->pass), (const uint8_t *) cfg->pass)) != 0) {
    return false;
  }

  v = cfg->channel;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_CHANNEL, 1,
                        (uint8_t *) &v)) != 0) {
    return false;
  }

  v = cfg->hidden;
  if ((ret = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_HIDDEN_SSID, 1,
                        (uint8_t *) &v)) != 0) {
    return false;
  }

  sl_NetAppStop(SL_NETAPP_DHCP_SERVER_ID);

  memset(&ipcfg, 0, sizeof(ipcfg));
#if SL_MAJOR_VERSION_NUM >= 2
  if (!inet_pton(AF_INET, cfg->ip, &ipcfg.Ip) ||
      !inet_pton(AF_INET, cfg->netmask, &ipcfg.IpMask) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.IpGateway) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.IpDnsServer) ||
      (ret = sl_NetCfgSet(SL_NETCFG_IPV4_AP_ADDR_MODE, SL_NETCFG_ADDR_STATIC,
                          sizeof(ipcfg), (uint8_t *) &ipcfg)) != 0) {
    return false;
  }
#else
  if (!inet_pton(AF_INET, cfg->ip, &ipcfg.ipV4) ||
      !inet_pton(AF_INET, cfg->netmask, &ipcfg.ipV4Mask) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.ipV4Gateway) ||
      !inet_pton(AF_INET, cfg->gw, &ipcfg.ipV4DnsServer) ||
      (ret = sl_NetCfgSet(SL_IPV4_AP_P2P_GO_STATIC_ENABLE,
                          IPCONFIG_MODE_ENABLE_IPV4, sizeof(ipcfg),
                          (uint8_t *) &ipcfg)) != 0) {
    return false;
  }
#endif

  memset(&dhcpcfg, 0, sizeof(dhcpcfg));
  dhcpcfg.lease_time = 900;
  if (!inet_pton(AF_INET, cfg->dhcp_start, &dhcpcfg.ipv4_addr_start) ||
      !inet_pton(AF_INET, cfg->dhcp_end, &dhcpcfg.ipv4_addr_last) ||
      (ret =
           sl_NetAppSet(SL_NETAPP_DHCP_SERVER_ID, SL_NETAPP_DHCP_SRV_BASIC_OPT,
                        sizeof(dhcpcfg), (uint8_t *) &dhcpcfg)) != 0) {
    return false;
  }

  /* Turning the device off and on for the change to take effect. */
  if (!restart_nwp(ROLE_AP)) return false;

  if ((ret = sl_NetAppStart(SL_NETAPP_DHCP_SERVER_ID)) != 0) {
    LOG(LL_ERROR, ("DHCP server failed to start: %d", ret));
  }

  sl_WlanRxStatStart();

  LOG(LL_INFO, ("AP %s configured", ssid));

  (void) ret;

  return true;
}

bool mgos_wifi_dev_sta_connect(void) {
  int ret;
  ret = sl_WlanConnect(
      (const _i8 *) s_sta_cfg.ssid, strlen(s_sta_cfg.ssid), 0, &s_sta_cfg.sp,
      (s_sta_cfg.sp.Type == SL_WLAN_SEC_TYPE_WPA_ENT ? &s_sta_cfg.spext
                                                     : NULL));
  if (ret != 0) {
    LOG(LL_ERROR, ("sl_WlanConnect failed: %d", ret));
    return false;
  }

  sl_WlanRxStatStart();

  return true;
}

bool mgos_wifi_dev_sta_disconnect(void) {
  return (sl_WlanDisconnect() == 0);
}

bool mgos_wifi_dev_get_ip_info(int if_instance,
                               struct mgos_net_ip_info *ip_info) {
  int r = -1;
  SlNetCfgIpV4Args_t info = {0};
  SL_LEN_TYPE len = sizeof(info);
  SL_OPT_TYPE dhcp_is_on = 0;
  switch (if_instance) {
    case MGOS_NET_IF_WIFI_STA: {
      if (s_current_role != ROLE_STA) return false;
      r = sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &dhcp_is_on, &len,
                       (_u8 *) &info);
      break;
    }
    case MGOS_NET_IF_WIFI_AP: {
      if (s_current_role != ROLE_AP) return false;
      r = sl_NetCfgGet(SL_NETCFG_IPV4_AP_ADDR_MODE, &dhcp_is_on, &len,
                       (_u8 *) &info);
      break;
    }
    default:
      return false;
  }
  if (r < 0) {
    LOG(LL_ERROR, ("sl_NetCfgGet failed: %d", r));
    return false;
  }
#if SL_MAJOR_VERSION_NUM >= 2
  ip_info->ip.sin_addr.s_addr = ntohl(info.Ip);
  ip_info->netmask.sin_addr.s_addr = ntohl(info.IpMask);
  ip_info->gw.sin_addr.s_addr = ntohl(info.IpGateway);
#else
  ip_info->ip.sin_addr.s_addr = ntohl(info.ipV4);
  ip_info->netmask.sin_addr.s_addr = ntohl(info.ipV4Mask);
  ip_info->gw.sin_addr.s_addr = ntohl(info.ipV4Gateway);
#endif
  return true;
}

static char *ip2str(uint32_t ip) {
  char *ipstr = NULL;
  mg_asprintf(&ipstr, 0, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(ip, 3),
              SL_IPV4_BYTE(ip, 2), SL_IPV4_BYTE(ip, 1), SL_IPV4_BYTE(ip, 0));
  return ipstr;
}

char *mgos_wifi_get_sta_default_dns(void) {
  SlNetCfgIpV4Args_t info = {0};
  SL_LEN_TYPE len = sizeof(info);
  SL_OPT_TYPE dhcp_is_on = 0;
  if (sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &dhcp_is_on, &len,
                   (_u8 *) &info) != 0 ||
      !dhcp_is_on) {
    return NULL;
  }
#if SL_MAJOR_VERSION_NUM >= 2
  return (info.IpDnsServer != 0 ? ip2str(info.IpDnsServer) : NULL);
#else
  return (info.ipV4DnsServer != 0 ? ip2str(info.ipV4DnsServer) : NULL);
#endif
}

bool mgos_wifi_dev_start_scan(void) {
  bool ret = false;
  int n = -1, num_res = 0, i, j;
  struct mgos_wifi_scan_result *res = NULL;
  SlWlanNetworkEntry_t info[2];

  if (!ensure_role_sta()) goto out;

  for (i = 0; (n = sl_WlanGetNetworkList(i, 2, info)) > 0; i += 2) {
    res = (struct mgos_wifi_scan_result *) realloc(
        res, (num_res + n) * sizeof(*res));
    if (res == NULL) {
      goto out;
    }
    for (j = 0; j < n; j++) {
      SlWlanNetworkEntry_t *e = &info[j];
      struct mgos_wifi_scan_result *r = &res[num_res];
      _u8 sec_type = 0;
#if SL_MAJOR_VERSION_NUM >= 2
      strncpy(r->ssid, (const char *) e->Ssid, sizeof(r->ssid));
      memcpy(r->bssid, e->Bssid, sizeof(r->bssid));
      r->rssi = e->Rssi;
      sec_type = SL_WLAN_SCAN_RESULT_SEC_TYPE_BITMAP(e->SecurityInfo);
#else
      strncpy(r->ssid, (const char *) e->ssid, sizeof(r->ssid));
      memcpy(r->bssid, e->bssid, sizeof(r->bssid));
      r->rssi = e->rssi;
      sec_type = e->sec_type;
#endif
      r->ssid[sizeof(r->ssid) - 1] = '\0';
      r->channel = 0; /* n/a */
      switch (sec_type) {
        case SL_WLAN_SECURITY_TYPE_BITMAP_OPEN:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_OPEN;
          break;
        case SL_WLAN_SECURITY_TYPE_BITMAP_WEP:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WEP;
          break;
        case SL_WLAN_SECURITY_TYPE_BITMAP_WPA:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA_PSK;
          break;
#if SL_MAJOR_VERSION_NUM >= 2
        case (SL_WLAN_SECURITY_TYPE_BITMAP_WPA |
              SL_WLAN_SECURITY_TYPE_BITMAP_WPA2):
#endif
        case SL_WLAN_SECURITY_TYPE_BITMAP_WPA2:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA2_PSK;
          break;
        default:
          LOG(LL_INFO, ("%s Unknown sec type: %d", r->ssid, sec_type));
          continue;
      }
      num_res++;
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

int mgos_wifi_sta_get_rssi(void) {
  if (s_current_role != ROLE_STA) return 0;
  SlWlanGetRxStatResponse_t rx_stats;
  _i16 r = sl_WlanRxStatGet(&rx_stats, 0);
  if (r == 0) {
    int s = 0, n = 0;
    if (rx_stats.AvarageDataCtrlRssi < 0) {
      s += rx_stats.AvarageDataCtrlRssi;
      n++;
    }
    if (rx_stats.AvarageMgMntRssi < 0) {
      s += rx_stats.AvarageMgMntRssi;
      n++;
    }
    return (n > 0 ? s / n : 0);
  } else {
    return 0;
  }
}

void mgos_wifi_dev_init(void) {
}

void mgos_wifi_dev_deinit(void) {
}
