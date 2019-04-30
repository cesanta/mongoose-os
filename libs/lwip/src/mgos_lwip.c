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

#include "mgos_lwip.h"

#include <stdbool.h>

#include "common/platform.h"

#include "lwip/tcpip.h"

bool mgos_lwip_if_get_ip_info(const struct netif *nif,
                              struct mgos_net_ip_info *ip_info) {
  if (nif == NULL) return false;
  ip_info->ip.sin_addr.s_addr = ip_addr_get_ip4_u32(&nif->ip_addr);
  ip_info->netmask.sin_addr.s_addr = ip_addr_get_ip4_u32(&nif->netmask);
  ip_info->gw.sin_addr.s_addr = ip_addr_get_ip4_u32(&nif->gw);
  return true;
}

static void tcpip_init_done(void *arg) {
  *((bool *) arg) = true;
}

bool mgos_lwip_init(void) {
#if CS_PLATFORM != CS_P_ESP32 && CS_PLATFORM != CS_P_ESP8266
  volatile bool lwip_inited = false;
  tcpip_init(tcpip_init_done, (void *) &lwip_inited);
  while (!lwip_inited) {
  }
#endif
  return true;
}
