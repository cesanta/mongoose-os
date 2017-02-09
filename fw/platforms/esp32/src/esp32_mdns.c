/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "common/platform.h"
#include "common/cs_dbg.h"

#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>

void mgos_mdns_hal_join_group(const char *group) {
  ip4_addr_t group_addr;
  group_addr.addr = inet_addr(group);

  LOG(LL_INFO, ("Joining multicast group %s", group));

  if (igmp_joingroup(IP4_ADDR_ANY, &group_addr) != ERR_OK) {
    LOG(LL_INFO, ("udp_join_multigrup failed!"));
  };
}
