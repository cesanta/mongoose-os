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

#include <stdlib.h>
#include <string.h>

#include "lwip/ip_addr.h"

#include "common/cs_dbg.h"

#include "mgos_sys_config.h"

const char *mgos_eth_speed_str(enum mgos_eth_speed speed) {
  switch (speed) {
    case MGOS_ETH_SPEED_AUTO:
      return "auto";
    case MGOS_ETH_SPEED_10M:
      return "10";
    case MGOS_ETH_SPEED_100M:
      return "100";
  }
  return "";
}

const char *mgos_eth_duplex_str(enum mgos_eth_duplex duplex) {
  switch (duplex) {
    case MGOS_ETH_DUPLEX_AUTO:
      return "auto";
    case MGOS_ETH_DUPLEX_HALF:
      return "half";
    case MGOS_ETH_DUPLEX_FULL:
      return "full";
  }
  return "";
}

bool mgos_eth_phy_opts_from_str(const char *str,
                                struct mgos_eth_phy_opts *opts) {
  if (str == NULL) return false;

  memset(opts, 0, sizeof(*opts));

  opts->autoneg_on = (strcmp(str, "auto") == 0);

  if (opts->autoneg_on) {
    return true;
  }

  size_t len = strlen(str);

  char *end = NULL;
  switch (strtol(str, &end, 10)) {
    case 10:
      opts->speed = MGOS_ETH_SPEED_10M;
      break;
    case 100:
      opts->speed = MGOS_ETH_SPEED_100M;
      break;
    default:
      return false;
  }

  if (len - (end - str) != 2) return false;
  if (strcmp(end, "FD")) {
    opts->duplex = MGOS_ETH_DUPLEX_FULL;
  } else if (strcmp(end, "HD")) {
    opts->duplex = MGOS_ETH_DUPLEX_HALF;
  } else {
    return false;
  }

  return true;
}

bool mgos_eth_get_static_ip_config(ip4_addr_t *ip, ip4_addr_t *netmask,
                                   ip4_addr_t *gw) {
  bool res = false;
  memset(ip, 0, sizeof(*ip));
  memset(netmask, 0, sizeof(*netmask));
  memset(gw, 0, sizeof(*gw));

  if (mgos_sys_config_get_eth_ip() == NULL) {
    res = true;
    goto clean;
  }

  if (!ip4addr_aton(mgos_sys_config_get_eth_ip(), ip)) {
    LOG(LL_ERROR, ("Invalid eth.ip!"));
    goto clean;
  }
  if (mgos_sys_config_get_eth_netmask() == NULL ||
      !ip4addr_aton(mgos_sys_config_get_eth_netmask(), netmask)) {
    LOG(LL_ERROR, ("Invalid eth.netmask!"));
    goto clean;
  }
  if (mgos_sys_config_get_eth_gw() != NULL &&
      !ip4addr_aton(mgos_sys_config_get_eth_gw(), gw)) {
    LOG(LL_ERROR, ("Invalid eth.gw!"));
    goto clean;
  }

  res = true;

clean:
  return res;
}
