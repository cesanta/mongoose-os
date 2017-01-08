/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#if CS_PLATFORM == CS_P_STM32

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>

/*
 * Fake declarations to get c_hello compiling w/out network
 * TODO(alashkin): remove this during working on stm32/cude networking
 */

typedef int sock_t;

struct sockaddr {
};

struct in_addr{
  int s_addr;
};

struct sockaddr_in {
  int sin_family;
  int sin_port;
  struct in_addr sin_addr;
};

#define INVALID_SOCKET -1
#define SOCK_DGRAM -1
#define SOCK_STREAM -1
#define AF_INET -1

#define to64(x) strtoll(x, NULL, 10)

#define INT64_FMT "ld"
#define SIZE_T_FMT "u"

#define htonl(x) (x)
#define htons(x) (x)
#define ntohs(x) (x)
#define ntohl(x) (x)

const char *inet_ntop(int af, const void *src, char *dst, int size);

#endif /* CS_PLATFORM == CS_P_STM32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_STM32_H_ */
