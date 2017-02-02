#include "stm32_lwip.h"
#include "fw/src/mgos_wifi.h"
#include "common/platform.h"
#include "common/cs_dbg.h"
#include "mongoose/mongoose.h"
#include "fw/src/mgos_wifi.h"
#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/netif.h>

extern struct netif gnetif;

int stm32_have_ip_address() {
  return gnetif.ip_addr.addr != 0;
}

char *stm32_get_ip_address() {
  return inet_ntoa(gnetif.ip_addr);
}

void stm32_finish_net_init() {
  /*
   * Temporary solution to get c_mqtt working
   * TODO(alashkin): write DD about ETH
   * and remove (modify) this
   */
  mgos_wifi_on_change_cb(MGOS_WIFI_CONNECTED);
  mgos_wifi_on_change_cb(MGOS_WIFI_IP_ACQUIRED);
}

void mgos_mdns_hal_join_group(const char *iface_ip, const char *group) {
  ip_addr_t host_addr;
  ip_addr_t group_addr;

  host_addr.addr = inet_addr(iface_ip);
  group_addr.addr = inet_addr(group);

  LOG(LL_INFO, ("Joining multicast group %s, %s", group, iface_ip));

  if (igmp_joingroup(&host_addr, &group_addr) != ERR_OK) {
    LOG(LL_INFO, ("udp_join_multigrup failed!"));
    goto clean;
  };

  netif_set_default(&gnetif);

clean:
  return;
}
