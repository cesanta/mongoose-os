/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_INCLUDE_LWIP_H_
#define CS_FW_PLATFORMS_STM32_INCLUDE_LWIP_H_

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "ethernetif.h"

#if WITH_RTOS
#include "lwip/tcpip.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern ETH_HandleTypeDef heth;

void MX_LWIP_Init(void);

void MX_LWIP_Process(void);

int stm32_have_ip_address();
char *stm32_get_ip_address();
void stm32_finish_net_init();

#ifdef __cplusplus
}
#endif
#endif /* CS_FW_PLATFORMS_STM32_INCLUDE_LWIP_H_ */
