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

#include "esp_eth.h"
#if defined(MGOS_ETH_PHY_LAN87x0)
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#define PHY_MODEL "LAN87x0"
#elif defined(MGOS_ETH_PHY_TLK110)
#include "eth_phy/phy_tlk110.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_tlk110_default_ethernet_config
#define PHY_MODEL "TLK110"
#else
#error Unknown/unspecified PHY model
#endif
#include "tcpip_adapter.h"

#include "lwip/ip_addr.h"

#include "common/cs_dbg.h"

#include "mgos_eth.h"
#include "mgos_gpio.h"
#include "mgos_net.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"

static void phy_device_power_enable_via_gpio(bool enable) {
  if (!enable) {
    /* Do the PHY-specific power_enable(false) function before powering down */
    DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(false);
  }

  const int pin = mgos_sys_config_get_eth_phy_pwr_gpio();
  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_write(pin, enable);

  /* Allow the power up/down to take effect, min 300us. */
  mgos_msleep(1);

  if (enable) {
    /* Run the PHY-specific power on operations now the PHY has power */
    DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(true);
  }
}

static void eth_config_pins(void) {
  phy_rmii_configure_data_interface_pins();
  phy_rmii_smi_configure_pins(mgos_sys_config_get_eth_mdc_gpio(),
                              mgos_sys_config_get_eth_mdio_gpio());
}

bool mgos_ethernet_init(void) {
  bool res = false;

  if (!mgos_sys_config_get_eth_enable()) {
    res = true;
    goto clean;
  }

  tcpip_adapter_ip_info_t static_ip;
  if (!mgos_eth_get_static_ip_config(&static_ip.ip, &static_ip.netmask,
                                     &static_ip.gw)) {
    goto clean;
  }

  eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
  const char *phy_model = PHY_MODEL;

  /* Set the PHY address in the example configuration */
  config.phy_addr = mgos_sys_config_get_eth_phy_addr();
  config.clock_mode = mgos_sys_config_get_eth_clk_mode();
  config.gpio_config = eth_config_pins;
  config.tcpip_input = tcpip_adapter_eth_input;

  if (mgos_sys_config_get_eth_phy_pwr_gpio() != -1) {
    config.phy_power_enable = phy_device_power_enable_via_gpio;
  }

  esp_err_t ret = esp_eth_init(&config);
  if (ret != ESP_OK) {
    LOG(LL_ERROR, ("Ethernet init failed: %d", ret));
    return false;
  }

  uint8_t mac[6];
  esp_eth_get_mac(mac);
  bool is_dhcp = ip4_addr_isany_val(static_ip.ip);

  LOG(LL_INFO,
      ("ETH: MAC %02x:%02x:%02x:%02x:%02x:%02x; PHY: %s @ %d%s", mac[0], mac[1],
       mac[2], mac[3], mac[4], mac[5], phy_model,
       mgos_sys_config_get_eth_phy_addr(), (is_dhcp ? "; IP: DHCP" : "")));
  if (!is_dhcp) {
    char ips[16], nms[16], gws[16];
    ip4addr_ntoa_r(&static_ip.ip, ips, sizeof(ips));
    ip4addr_ntoa_r(&static_ip.netmask, nms, sizeof(nms));
    ip4addr_ntoa_r(&static_ip.gw, gws, sizeof(gws));
    LOG(LL_INFO, ("ETH: IP %s/%s, GW %s", ips, nms, gws));
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
    if (tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &static_ip) != ESP_OK) {
      LOG(LL_ERROR, ("ETH: Failed to set ip info"));
      goto clean;
    }
  }

  res = (esp_eth_enable() == ESP_OK);

clean:
  return res;
}

bool mgos_eth_dev_get_ip_info(int if_instance,
                              struct mgos_net_ip_info *ip_info) {
  tcpip_adapter_ip_info_t info;
  if (if_instance != 0 ||
      tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &info) != ESP_OK ||
      info.ip.addr == 0) {
    return false;
  }
  ip_info->ip.sin_addr.s_addr = info.ip.addr;
  ip_info->netmask.sin_addr.s_addr = info.netmask.addr;
  ip_info->gw.sin_addr.s_addr = info.gw.addr;
  return true;
}
