/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#pragma once

#include <machine/endian.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHECKSUM_BY_HARDWARE 0

#define LWIP_DHCP 1
#define NO_SYS 0
#define TCPIP_THREAD_PRIO 10
#define TCPIP_THREAD_STACKSIZE 2048 /* x4 */
#define TCPIP_MBOX_SIZE 32
#define MEM_ALIGNMENT 4
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define MEMP_NUM_SYS_TIMEOUT 16
#define LWIP_ETHERNET 1
#define LWIP_MULTICAST_TX_OPTIONS 1
#define LWIP_IGMP 1
#define LWIP_DNS 1
#define LWIP_DNS_SECURE 7
#define TCP_MSL 5000  // To reduce TIME_WAIT length
#define TCP_SND_QUEUELEN 9
#define TCP_SNDLOWAT 1071
#define TCP_SNDQUEUELOWAT 5
#define TCP_WND_UPDATE_THRESHOLD 536
#define LWIP_NETCONN 0
#define LWIP_NETIF_API 1
#define LWIP_NETIF_EXT_STATUS_CALLBACK 1
#define LWIP_SOCKET 0
#define LWIP_COMPAT_SOCKETS 0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#define LWIP_STATS 0
#define LWIP_TCP_KEEPALIVE 1

#define SYS_LIGHTWEIGHT_PROT 1

#define LWIP_NOASSERT 1

#define LWIP_USE_EXTERNAL_MBEDTLS 1

#define LWIP_DEBUG 1

#include "common/cs_dbg.h"
#define LWIP_PLATFORM_DIAG(x) LOG(LL_DEBUG, x)
#define LWIP_PLATFORM_ERROR(x) LOG(LL_ERROR, (x))

// #define DHCP_DEBUG LWIP_DBG_ON

#ifdef __cplusplus
}
#endif
