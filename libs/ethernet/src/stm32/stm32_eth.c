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

#include "mgos_eth.h"

#include "common/cs_dbg.h"

#include "mgos_lwip.h"
#include "mgos_net.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"

#include "lwip/dhcp.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"

#include "stm32_eth_netif.h"
#include "stm32_eth_phy.h"

#include "stm32f7xx_hal.h"

static struct netif *s_eth_netif = NULL;

bool mgos_eth_dev_get_ip_info(int if_instance,
                              struct mgos_net_ip_info *ip_info) {
  if (if_instance != 0) return false;
  return mgos_lwip_if_get_ip_info(s_eth_netif, ip_info);
}

bool mgos_ethernet_init(void) {
  bool res = false;
  char *mac_str = NULL;
  struct mgos_eth_opts opts;

  if (!mgos_sys_config_get_eth_enable()) {
    res = true;
    goto clean;
  }

  memset(&opts, 0, sizeof(opts));

  if (!mgos_eth_phy_opts_from_str(mgos_sys_config_get_eth_speed(),
                                  &opts.phy_opts)) {
    LOG(LL_ERROR, ("Invalid eth.speed"));
    goto clean;
  }

  mac_str = strdup(mgos_sys_config_get_eth_mac());

  if (strchr(mac_str, '?') != NULL) {
    uint8_t dev_mac[6] = {0};
    char dev_mac_hex[sizeof(dev_mac) * 2];
    device_get_mac_address(dev_mac);
    cs_to_hex(dev_mac_hex, dev_mac, sizeof(dev_mac));
    struct mg_str s = mg_mk_str(mac_str);
    mgos_expand_placeholders(mg_mk_str_n(dev_mac_hex, sizeof(dev_mac_hex)), &s);
  }
  {
    unsigned int mac[6] = {0};
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3],
               &mac[4], &mac[5]) != 6) {
      LOG(LL_ERROR, ("Invalid eth.mac!"));
      goto clean;
    }
    opts.mac[0] = mac[0];
    opts.mac[1] = mac[1];
    opts.mac[2] = mac[2];
    opts.mac[3] = mac[3];
    opts.mac[4] = mac[4];
    opts.mac[5] = mac[5];
  }

  opts.phy_addr = mgos_sys_config_get_eth_phy_addr();
  opts.mtu = mgos_sys_config_get_eth_mtu();

  ip4_addr_t ip, netmask, gw;
  if (!mgos_eth_get_static_ip_config(&ip, &netmask, &gw)) {
    goto clean;
  }

  struct netif *netif = (struct netif *) calloc(1, sizeof(*netif));
  if (netif_add(netif, &ip, &netmask, &gw, &opts, stm32_eth_netif_init,
                ethernet_input) == NULL) {
    LOG(LL_ERROR, ("Failed to init eth interface"));
    goto clean;
  }
  netif_set_up(netif);
  bool is_dhcp = ip4_addr_isany_val(ip);
  LOG(LL_INFO,
      ("ETH: MAC %02x:%02x:%02x:%02x:%02x:%02x; "
       "PHY: %s @ %d, speed %s, duplex %s%s",
       netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3],
       netif->hwaddr[4], netif->hwaddr[5], stm32_eth_phy_name(), opts.phy_addr,
       mgos_eth_speed_str(opts.phy_opts.speed),
       mgos_eth_duplex_str(opts.phy_opts.duplex),
       (is_dhcp ? "; IP: DHCP" : "")));
  if (is_dhcp) {
    dhcp_start(netif);
  } else {
    char ips[16], nms[16], gws[16];
    ip4addr_ntoa_r(&netif->ip_addr, ips, sizeof(ips));
    ip4addr_ntoa_r(&netif->netmask, nms, sizeof(nms));
    ip4addr_ntoa_r(&netif->gw, gws, sizeof(gws));
    LOG(LL_INFO, ("ETH: IP %s/%s, GW %s", ips, nms, gws));
    if (!ip4_addr_isany_val(gw)) {
      netif_set_default(netif);
    }
  }

  s_eth_netif = netif;

  res = true;

clean:
  free(mac_str);
  if (!res) {
    free(s_eth_netif);
    s_eth_netif = NULL;
  }
  return res;
}
