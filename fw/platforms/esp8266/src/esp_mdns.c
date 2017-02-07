#include <stdlib.h>

#include "common/platform.h"
#include "fw/src/mgos_wifi.h"
#include "common/cs_dbg.h"

#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>
struct ieee80211_conn; /* it seems that espressif didn't export this */
#include <netif/wlan_lwip_if.h>

void mgos_mdns_hal_join_group(const char *group) {
  struct ip_addr group_addr;
  group_addr.addr = inet_addr(group);

  LOG(LL_INFO, ("Joining multicast group %s", group));

  if (igmp_joingroup(IP_ADDR_ANY, &group_addr) != ERR_OK) {
    LOG(LL_INFO, ("udp_join_multigrup failed!"));
  };
}
