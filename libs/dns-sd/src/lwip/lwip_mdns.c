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

#include <stdlib.h>

#include "common/platform.h"
#include "common/cs_dbg.h"

#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>

#ifndef ip_2_ip4
#define ip4_addr_t struct ip_addr
#endif

#ifndef IP4_ADDR_ANY
#define IP4_ADDR_ANY IP_ADDR_ANY
#endif

void mgos_mdns_hal_join_group(const char *group) {
  ip4_addr_t group_addr;
  group_addr.addr = inet_addr(group);

  LOG(LL_INFO, ("Joining multicast group %s", group));

#ifdef IP4_ADDR_ANY4
#define ADDR IP4_ADDR_ANY4
#else
#define ADDR IP4_ADDR_ANY
#endif

  if (igmp_joingroup(ADDR, &group_addr) != ERR_OK) {
    LOG(LL_INFO, ("udp_join_multigroup failed!"));
  };
}
