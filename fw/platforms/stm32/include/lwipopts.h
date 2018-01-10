/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_INCLUDE_LWIPOPTS_H_
#define CS_FW_PLATFORMS_STM32_INCLUDE_LWIPOPTS_H_

#include "stm32f7xx_hal.h"

#include <machine/endian.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t u32_t;

#define CHECKSUM_BY_HARDWARE 0

#define LWIP_DHCP 1
#define NO_SYS 0
#define TCPIP_THREAD_PRIO 10
#define TCPIP_THREAD_STACKSIZE 2048 /* x4 */
#define TCPIP_MBOX_SIZE 32
#define MEM_ALIGNMENT 4
#define MEM_LIBC_MALLOC 1
#define MEMP_NUM_SYS_TIMEOUT 6
#define LWIP_ETHERNET 1
#define LWIP_MULTICAST_TX_OPTIONS 1
#define LWIP_IGMP 1
#define LWIP_DNS_SECURE 7
#define TCP_SND_QUEUELEN 9
#define TCP_SNDLOWAT 1071
#define TCP_SNDQUEUELOWAT 5
#define TCP_WND_UPDATE_THRESHOLD 536
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_STATS 0

#define SYS_LIGHTWEIGHT_PROT 0

#ifdef __cplusplus
}
#endif
#endif /* CS_FW_PLATFORMS_STM32_INCLUDE_LWIPOPTS_H_ */
