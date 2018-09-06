/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
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

#include <stdint.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bits and bobs necessary to build Mongoose w/o LwIP.
 * XXX: This is not specific to STM32 and does not belong here.
 * TODO(rojer): generalize and move somewhere else.
 */
#if !defined(MG_LWIP) || !MG_LWIP

typedef uint8_t sa_family_t;
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
  in_addr_t s_addr;
};

struct in6_addr {
  union {
    uint32_t u32_addr[4];
    uint8_t u8_addr[16];
  } un;
#define s6_addr un.u8_addr
};

/* members are in network byte order */
struct sockaddr_in {
  uint8_t sin_len;
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
  char sin_zero[SIN_ZERO_LEN];
};

struct sockaddr_in6 {
  uint8_t sin6_len;          /* length of this structure    */
  sa_family_t sin6_family;   /* AF_INET6                    */
  in_port_t sin6_port;       /* Transport layer port #      */
  uint32_t sin6_flowinfo;    /* IPv6 flow information       */
  struct in6_addr sin6_addr; /* IPv6 address                */
  uint32_t sin6_scope_id;    /* Set of interfaces for scope */
};

struct sockaddr {
  uint8_t sa_len;
  sa_family_t sa_family;
  char sa_data[14];
};

typedef int sock_t;
#define INVALID_SOCKET -1
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

uint32_t swap_bytes_32(uint32_t x);
uint16_t swap_bytes_16(uint16_t x);

#define htons(x) swap_bytes_16(x)
#define ntohs(x) swap_bytes_16(x)
#define htonl(x) swap_bytes_32(x)
#define ntohl(x) swap_bytes_32(x)

char *inet_ntoa(struct in_addr in);
const char *inet_ntop(int af, const void *src, char *dst, int size);

#endif /* !MG_LWIP */

#ifdef __cplusplus
}
#endif
