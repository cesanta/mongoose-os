/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "stm32_lwip.h"
#include "common/platform.h"
#include "common/cs_dbg.h"
#include "mongoose/mongoose.h"

#include "ethernetif.h"

#include <lwip/dhcp.h>
#include <lwip/igmp.h>
#include <lwip/inet.h>
#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/timeouts.h>
#include <netif/etharp.h>

#include "mgos_mongoose_internal.h"

extern struct netif gnetif;

int stm32_have_ip_address() {
  return gnetif.ip_addr.addr != 0;
}

char *stm32_get_ip_address() {
  return inet_ntoa(gnetif.ip_addr);
}

void stm32_finish_net_init() {
  //  mgos_wifi_on_change_cb(MGOS_WIFI_CONNECTED);
  //  mgos_wifi_on_change_cb(MGOS_WIFI_IP_ACQUIRED);
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

char *mgos_wifi_get_sta_default_gw() {
  /* TODO(alashkin): implement */
  return NULL;
}

void Error_Handler(void);

uint32_t DHCPfineTimer = 0;
uint32_t DHCPcoarseTimer = 0;

struct netif gnetif;

ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

void MX_LWIP_Init(void) {
  /* Initilialize the LwIP stack without RTOS */
  lwip_init();

  /* IP addresses initialization with DHCP (IPv4) */
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;

  /* add the network interface (IPv4/IPv6) without RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
            &ethernet_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif)) {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  } else {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }

  /* Start DHCP negotiation for a network interface (IPv4) */
  dhcp_start(&gnetif);
}

void MX_LWIP_Process(void) {
  ethernetif_input(&gnetif);

/* Handle timeouts */
#if LWIP_TIMERS
  sys_check_timeouts();
#endif
}
