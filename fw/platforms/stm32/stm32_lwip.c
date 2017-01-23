#include "stm32_lwip.h"
#include "lwip/netif.h"
#include "mongoose/mongoose.h"
#include "fw/src/mgos_wifi.h"

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
