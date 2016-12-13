/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "common/cs_dbg.h"
#include "system_config.h"
#include "system_definitions.h"

enum {
  PIC32_ETHERNET_STATE_INIT,
  PIC32_ETHERNET_STATE_WAIT_FOR_IP,
  PIC32_ETHERNET_STATE_CONNECTED,
  PIC32_ETHERNET_STATE_IDLE,
}

static state;

void pic32_ethernet_init(void) {
  state = PIC32_ETHERNET_STATE_INIT;
}

void pic32_ethernet_poll(void) {
  SYS_STATUS tcpip_status;
  const char *net_name, *net_bios_name;
  static IPV4_ADDR last_ip_addr[2] = {{-1}, {-1}};
  IPV4_ADDR ip_addr;
  TCPIP_NET_HANDLE neth;
  int i, nnets;

  switch (state) {
    case PIC32_ETHERNET_STATE_INIT: {
      tcpip_status = TCPIP_STACK_Status(sysObj.tcpip);
      if (tcpip_status < 0) {  // some error occurred
        LOG(LL_ERROR,
            ("APP: TCP/IP stack initialization failed! %d", tcpip_status));
        state = PIC32_ETHERNET_STATE_IDLE;
      } else if (tcpip_status == SYS_STATUS_READY) {
        /*
         * now that the stack is ready we can check the available interfaces
         */
        nnets = TCPIP_STACK_NumberOfNetworksGet();
        for (i = 0; i < nnets; i++) {
          neth = TCPIP_STACK_IndexToNet(i);
          net_name = TCPIP_STACK_NetNameGet(neth);
          net_bios_name = TCPIP_STACK_NetBIOSName(neth);

#if defined(TCPIP_STACK_USE_NBNS)
          LOG(LL_INFO, ("Interface %s on host %s - NBNS enabled", net_name,
                        net_bios_name));
#else
          LOG(LL_INFO, ("Interface %s on host %s - NBNS disabled", net_name,
                        net_bios_name));
#endif
        }
        state = PIC32_ETHERNET_STATE_WAIT_FOR_IP;
      }
      break;
    }

    case PIC32_ETHERNET_STATE_WAIT_FOR_IP:
      nnets = TCPIP_STACK_NumberOfNetworksGet();
      for (i = 0; i < nnets; i++) {
        neth = TCPIP_STACK_IndexToNet(i);
        ip_addr.Val = TCPIP_STACK_NetAddress(neth);
        if (last_ip_addr[i].Val != ip_addr.Val) {
          last_ip_addr[i].Val = ip_addr.Val;

          LOG(LL_INFO,
              ("%s IP Address: %d.%d.%d.%d", TCPIP_STACK_NetNameGet(neth),
               ip_addr.v[0], ip_addr.v[1], ip_addr.v[2], ip_addr.v[3]));

          /* Wait for a valid IP */
          if (ip_addr.v[0] != 0 && ip_addr.v[0] != 169) {
            LOG(LL_INFO, ("Connected"));
            state = PIC32_ETHERNET_STATE_CONNECTED;
          }
        }
      }
      break;

    case PIC32_ETHERNET_STATE_CONNECTED:
      /* TODO(dfrank) : add reconnect logic */
      break;

    case PIC32_ETHERNET_STATE_IDLE:
      break;

    default:
      /* should never be here */
      abort();
      break;
  }
}
