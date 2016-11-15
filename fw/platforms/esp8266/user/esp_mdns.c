#include <stdlib.h>

#include "common/platform.h"
#include "fw/src/miot_wifi.h"
#include "common/cs_dbg.h"

#include <lwip/igmp.h>
#include <lwip/inet.h>
struct ieee80211_conn; /* it seems that espressif didn't export this */
#include <netif/wlan_lwip_if.h>

void miot_mdns_hal_join_group(const char *iface_ip, const char *group) {
  struct ip_addr host_addr;
  struct ip_addr group_addr;

  host_addr.addr = inet_addr(iface_ip);
  group_addr.addr = inet_addr(group);

  LOG(LL_INFO, ("Joining multicast group %s", group));

  if (igmp_joingroup(&host_addr, &group_addr) != ERR_OK) {
    LOG(LL_INFO, ("udp_join_multigrup failed!"));
    goto clean;
  };

  // set station as default multicast interface
  struct netif *sta_netif = NULL;
  sta_netif = (struct netif *) eagle_lwip_getif(0x00);
  netif_set_default(sta_netif);

clean:
  return;
}
