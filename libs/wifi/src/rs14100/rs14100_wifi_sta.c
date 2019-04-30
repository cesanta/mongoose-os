/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/mg_str.h"

#include "mgos.h"
#include "mgos_lwip.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_wifi.h"
#include "mgos_wifi_hal.h"

#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/prot/dhcp.h"
#include "netif/etharp.h"
#include "netif/ethernet.h"

#include "rs14100_sdk.h"
#include "rs14100_wifi.h"

struct rs14100_sta_ctx {
  struct mg_str ssid, pass;
  ip4_addr_t ip, netmask, gw;
  struct netif *netif;
  bool connecting, connected, want_connected;
  bool dhcp_enabled, waiting_dhcp;
  uint8_t sta_bssid[6];
  int sta_channel;
};

struct rs14100_sta_ctx s_sta_ctx;

static void rs14100_wifi_sta_join_cb_tcpip(void *arg) {
  uint16_t status = (uintptr_t) arg;
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  bool ok = (status == RSI_SUCCESS);
  ctx->connecting = false;
  if (ok && !ctx->connected) {
    ctx->connected = true;
    if (!ctx->want_connected) {
      // This connection should be dropped.
      mgos_wifi_dev_sta_disconnect();
      return;
    }

    // Apply bg scan and roaming settings.
    rsi_wlan_execute_post_connect_cmds();

    netif_set_link_up(ctx->netif);
    struct mgos_wifi_dev_event_info dei = {
        .ev = MGOS_WIFI_EV_STA_CONNECTED,
        .sta_connected =
            {
             .channel = ctx->sta_channel,
            },
    };
    memcpy(dei.sta_connected.bssid, ctx->sta_bssid, 6);
    mgos_wifi_dev_event_cb(&dei);
    if (ctx->dhcp_enabled) {
      dhcp_start(ctx->netif);
      ctx->waiting_dhcp = true;
    } else {
      // Static IP, interface is ready.
      netif_set_default(ctx->netif);
      dei.ev = MGOS_WIFI_EV_STA_IP_ACQUIRED;
      mgos_wifi_dev_event_cb(&dei);
    }
  } else if (ok && ctx->connected) {
    // When roaming, we want to trigger DHCP renewal just in case
    // we roamed onto a network with the same SSID but different IP settings.
    struct dhcp *dhcp = ((struct dhcp *) netif_get_client_data(
        ctx->netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP));
    if (dhcp->state != DHCP_STATE_OFF) {
      dhcp_rebind(ctx->netif);
    }
  } else {
    ctx->connected = false;
    LOG(LL_ERROR, ("Join error: 0x%x", status));
    netif_set_link_down(ctx->netif);
    struct mgos_wifi_dev_event_info dei = {
        .ev = MGOS_WIFI_EV_STA_DISCONNECTED,
        .sta_disconnected =
            {
             .reason = status,
            },
    };
    mgos_wifi_dev_event_cb(&dei);
  }
}

static void rs14100_wifi_sta_join_cb(uint16_t status, const uint8_t *buffer,
                                     const uint16_t length) {
  void *arg = (void *) (uintptr_t) status;
  tcpip_callback(rs14100_wifi_sta_join_cb_tcpip, arg);
  (void) buffer;
  (void) length;
}

static void rs14100_wifi_sta_input_tcpip(void *arg) {
  struct pbuf *p = (struct pbuf *) arg;
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  err_t err = ctx->netif->input(p, ctx->netif);
  if (err != ERR_OK) {
    LOG(LL_ERROR, ("input error %d", err));
    pbuf_free(p);
    return;
  }
}

void rs14100_wifi_sta_ext_cb_tcpip(struct netif *netif,
                                   netif_nsc_reason_t reason,
                                   const netif_ext_callback_args_t *args) {
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  if (netif != ctx->netif) return;
  if (ctx->dhcp_enabled && (reason & LWIP_NSC_IPV4_ADDRESS_CHANGED) &&
      !ip4_addr_isany_val(ctx->netif->ip_addr)) {
    ctx->waiting_dhcp = false;
    netif_set_default(netif);
    struct mgos_wifi_dev_event_info dei = {
        .ev = MGOS_WIFI_EV_STA_IP_ACQUIRED,
    };
    mgos_wifi_dev_event_cb(&dei);
  }
  (void) args;
}

void rs14100_wifi_sta_input(const uint8_t *buffer, uint32_t length) {
  struct pbuf *p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
  if (p == NULL) {
    LOG(LL_ERROR, ("out of bufs"));
    return;
  }
  if (pbuf_take(p, buffer, length) != ERR_OK) return;
  if (tcpip_callback(rs14100_wifi_sta_input_tcpip, p)) {
    pbuf_free(p);
  }
}

// Like rsi_wlan_send_data, but takes data from a pbuf.
static err_t rs14100_wifi_sta_send_data(struct netif *netif, struct pbuf *p) {
  err_t res = ERR_OK;

  rsi_wlan_cb_t *wlan_cb = rsi_driver_cb->wlan_cb;

  rsi_mutex_lock(&wlan_cb->wlan_mutex);

  rsi_pkt_t *pkt = rsi_pkt_alloc(&wlan_cb->wlan_tx_pool);
  if (pkt == NULL) {
    LOG(LL_ERROR, ("no pkt"));
    res = ERR_MEM;
    goto out;
  }

  uint8_t *host_desc = pkt->desc;
  memset(host_desc, 0, RSI_HOST_DESC_LENGTH);
  rsi_uint16_to_2bytes(host_desc, p->tot_len);
  host_desc[1] |= (RSI_WLAN_DATA_Q << 4);

  pbuf_copy_partial(p, pkt->data, p->tot_len, 0);

  rsi_enqueue_pkt(&rsi_driver_cb->wlan_tx_q, pkt);
  wlan_cb->expected_response = RSI_WLAN_RSP_ASYNCHRONOUS;
  rsi_set_event(RSI_TX_EVENT);
  rsi_semaphore_wait(&wlan_cb->wlan_sem, RSI_WAIT_FOREVER);

  wlan_cb->expected_response = RSI_WLAN_RSP_CLEAR;
  rsi_pkt_free(&wlan_cb->wlan_tx_pool, pkt);

  res = ERR_OK;

out:
  rsi_mutex_unlock(&wlan_cb->wlan_mutex);
  (void) netif;
  return res;
}

static err_t rs14100_wifi_sta_netif_init(struct netif *netif) {
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "mos";
#endif /* LWIP_NETIF_HOSTNAME */
  netif->name[0] = 'w';
  netif->name[1] = 'l';
  netif->mtu = 1500;
  netif->hwaddr_len = 6;
  device_get_mac_address(netif->hwaddr);
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  netif->output = etharp_output;

  netif->linkoutput = rs14100_wifi_sta_send_data;

  return ERR_OK;
}

// See PRM 11.68
static void rs14100_wifi_state_notification_handler(uint16_t status,
                                                    uint8_t *buffer,
                                                    const uint32_t length) {
  if (status != RSI_SUCCESS) return;
  const rsi_state_notificaton_t *sn = (rsi_state_notificaton_t *) buffer;
  if (sn->rssi_val != 100) {
    LOG(LL_DEBUG, ("WiFi STA state: 0x%02x 0x%02x "
                   "bssid %02x:%02x:%02x:%02x:%02x:%02x ch %d RSSI %d",
                   sn->state_code, sn->reason_code, sn->bssid[0], sn->bssid[1],
                   sn->bssid[2], sn->bssid[3], sn->bssid[4], sn->bssid[5],
                   sn->channel, -sn->rssi_val));
  } else {
    LOG(LL_DEBUG,
        ("WiFi STA state: 0x%02x 0x%02x", sn->state_code, sn->reason_code));
  }
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  switch (sn->state_code & 0xf0) {
    case 0x00:
      memset(ctx->sta_bssid, 0, sizeof(ctx->sta_bssid));
      ctx->sta_channel = 0;
      break;
    case 0x90:
      // The module sends 0x9x status updates when disconnected but
      // reconnecting.
      // When it gives up (approx. 20 seconds with current rejoin settings),
      // it will send join response so here we do nothing.
      break;
    case 0x80:
      memcpy(ctx->sta_bssid, sn->bssid, sizeof(ctx->sta_bssid));
      ctx->sta_channel = sn->channel;
      if (ctx->connected) {  // Roaming.
        LOG(LL_INFO, ("WiFi STA: Moved to BSSID %02x:%02x:%02x:%02x:%02x:%02x "
                      "ch %d RSSI %d",
                      sn->bssid[0], sn->bssid[1], sn->bssid[2], sn->bssid[3],
                      sn->bssid[4], sn->bssid[5], sn->channel, -sn->rssi_val));
        rs14100_wifi_sta_join_cb(status, NULL, 0);
      }
      break;
  }
  (void) length;
}

bool mgos_wifi_dev_sta_setup(const struct mgos_config_wifi_sta *cfg) {
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;

  rsi_wlan_register_callbacks(RSI_WLAN_STATE_NOTIFICATION_HANDLER,
                              rs14100_wifi_state_notification_handler);

  if (!cfg->enable) {
    return true;
  }

  bool res = false;

  if (ctx->connecting) {
    // Previous join request is still active, can't do anything.
    return false;
  }

  rsi_rsp_wireless_info_t wi = {0};
  if (rsi_wlan_get(RSI_WLAN_INFO, (uint8_t *) &wi, sizeof(wi)) != RSI_SUCCESS) {
    goto out;
  }
  // If we were connected, disconnect now.
  // Could possibly optimize to see if we are already connected to the same
  // network as we want.
  if (wi.wlan_state != 0) {
    mgos_wifi_dev_sta_disconnect();
  }

  mg_strfree(&ctx->ssid);
  ctx->ssid = mg_strdup_nul(mg_mk_str(cfg->ssid));

  mg_strfree(&ctx->pass);
  ctx->pass = mg_strdup_nul(mg_mk_str(cfg->pass));

  if (cfg->ip != NULL && cfg->netmask != NULL) {
    ctx->dhcp_enabled = false;
    if (!ip4addr_aton(cfg->ip, &ctx->ip)) {
      LOG(LL_ERROR, ("Invalid %s!", "ip"));
      goto out;
    }
    if (!ip4addr_aton(cfg->netmask, &ctx->netmask)) {
      LOG(LL_ERROR, ("Invalid %s!", "netmask"));
      goto out;
    }
    if (cfg->gw != NULL && !ip4addr_aton(cfg->gw, &ctx->gw)) {
      LOG(LL_ERROR, ("Invalid %s!", "gw"));
      goto out;
    }
  } else {
    ctx->dhcp_enabled = true;
    memset(&ctx->ip, 0, sizeof(ctx->ip));
    memset(&ctx->netmask, 0, sizeof(ctx->netmask));
    memset(&ctx->gw, 0, sizeof(ctx->gw));
  }

  if (ctx->netif == NULL) {
    struct netif *netif = (struct netif *) calloc(1, sizeof(*ctx->netif));
    if (netif == NULL) goto out;
    if (netifapi_netif_add(netif, &ctx->ip, &ctx->netmask, &ctx->gw, ctx,
                           rs14100_wifi_sta_netif_init,
                           ethernet_input) != ERR_OK) {
      free(netif);
      goto out;
    }
    ctx->netif = netif;
  }

  netifapi_netif_set_up(ctx->netif);
  netifapi_netif_set_link_down(ctx->netif);

  res = true;

out:
  return res;
}

bool mgos_wifi_dev_sta_connect(void) {
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  rsi_security_mode_t sec = RSI_OPEN;
  if (ctx->pass.len > 0) {
    sec = RSI_WPA_WPA2_MIXED;
  }
  ctx->waiting_dhcp = false;
  int32_t status = rsi_wlan_connect_async(ctx->ssid.p, sec, ctx->pass.p,
                                          rs14100_wifi_sta_join_cb);
  if (status != RSI_SUCCESS) {
    if (status != RSI_ERROR_WLAN_NO_AP_FOUND) {
      LOG(LL_ERROR, ("join error 0x%lx %ld", status, status));
      return false;
    }
    // Ok, so no AP was found at this point. We'll let reconnect deal with this.
    struct mgos_wifi_dev_event_info dei = {
        .ev = MGOS_WIFI_EV_STA_DISCONNECTED,
        .sta_disconnected =
            {
             .reason = status,
            },
    };
    mgos_wifi_dev_event_cb(&dei);
  } else {
    ctx->want_connected = true;
    ctx->connecting = true;
  }
  return true;
}

bool mgos_wifi_dev_sta_disconnect(void) {
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  ctx->want_connected = false;
  // If DHCP client was active, stop it. If not, this is a no-op.
  if (ctx->netif != NULL) {
    dhcp_release_and_stop(ctx->netif);
    netifapi_netif_set_down(ctx->netif);
  }
  if (ctx->connected) {
    rsi_wlan_disconnect();  // Will trigger state change callback.
    netifapi_netif_set_link_down(ctx->netif);
    ctx->connected = false;
  }
  return true;
}

bool rs14100_wifi_sta_get_ip_info(struct mgos_net_ip_info *ip_info) {
  return mgos_lwip_if_get_ip_info(s_sta_ctx.netif, ip_info);
}

int mgos_wifi_sta_get_rssi(void) {
  rsi_rsp_rssi_t rsp;
  if (rsi_wlan_get(RSI_RSSI, (uint8_t *) &rsp, sizeof(rsp)) != RSI_SUCCESS) {
    return 0;
  }
  return -rsi_bytes2R_to_uint16(rsp.rssi_value);
}

static void rs14100_wifi_scan_cb(uint16_t status, const uint8_t *buffer,
                                 const uint16_t length) {
  rsi_rsp_scan_t *sr = (rsi_rsp_scan_t *) buffer;
  if (status != RSI_SUCCESS) {
    if (status == RSI_ERROR_WLAN_NO_AP_FOUND) {
      mgos_wifi_dev_scan_cb(0, NULL);
    } else {
      mgos_wifi_dev_scan_cb(-status, NULL);
    }
    return;
  }
  int num_res = 0;
  uint32_t count = rsi_bytes4R_to_uint32(sr->scan_count);
  struct mgos_wifi_scan_result *res = calloc(count, sizeof(*res));
  if (res != NULL) {
    for (uint32_t i = 0; i < count; i++) {
      const rsi_scan_info_t *si = &sr->scan_info[i];
      struct mgos_wifi_scan_result *r = res + num_res;
      r->channel = si->rf_channel;
      switch (si->security_mode) {
        case 0:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_OPEN;
          break;
        case 1:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA_PSK;
          break;
        case 2:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA2_PSK;
          break;
        case 3:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WEP;
          break;
        case 5:
          r->auth_mode = MGOS_WIFI_AUTH_MODE_WPA2_ENTERPRISE;
          break;
        default:
          continue;
      }
      memcpy(r->bssid, si->bssid, sizeof(r->bssid));
      memcpy(r->ssid, si->ssid, sizeof(r->ssid));
      r->rssi = -si->rssi_val;
      num_res++;
    }
  }
  mgos_wifi_dev_scan_cb(num_res, res);
  (void) length;
}

bool mgos_wifi_dev_start_scan(void) {
  struct rs14100_sta_ctx *ctx = &s_sta_ctx;
  if (ctx->connecting) return false;  // Already busy scanning.
  int32_t status;
  // When already connected use BG scan.
  if (ctx->connected) {
    status = rsi_wlan_bgscan_async(rs14100_wifi_scan_cb);
  } else {
    status = rsi_wlan_scan_async(NULL, 0, rs14100_wifi_scan_cb);
  }
  switch (status) {
    case RSI_ERROR_WLAN_NO_AP_FOUND:
      mgos_wifi_dev_scan_cb(0, NULL);
    // fallthrough
    case RSI_SUCCESS:
      return true;
    default:
      LOG(LL_ERROR, ("rsi_wlan_scan_async returned 0x%lx", status));
      return false;
  }
}

char *mgos_wifi_get_sta_default_dns(void) {
  const ip_addr_t *dns_ip = dns_getserver(0);
  if (dns_ip->addr == 0) return NULL;
  struct sockaddr_in a = {
      .sin_addr.s_addr = ip_addr_get_ip4_u32(dns_ip),
  };
  char dns_ip_str[16];
  mgos_net_ip_to_str(&a, dns_ip_str);
  return strdup(dns_ip_str);
}
