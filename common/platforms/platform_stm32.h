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

struct in_addr{
  int s_addr;
};

struct sockaddr {
  int sa_family;
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

#define F_GETFL -1
#define F_SETFL -1
#define O_NONBLOCK -1
#define fcntl(x1, x2, x3) (-1)
#define socket(x1, x2, x3) (-1)
#define connect(x1, x2, x3) (-1)
#define setsockopt(x1, x2, x3, x4, x5) (-1)
#define closesocket(x)
typedef int socklen_t;
#define accept(x1, x2, x3) (-1)
#define SOMAXCONN -1
#define listen(x1, x2) (-1)
#define bind(x1, x2, x3) (-1)
#define getsockname(x1, x2, x3) (-1)
#define sendto(x1, x2, x3, x4, x5, x6) (-1)
#define send(x1, x2, x3, x4) (-1)
#define recvfrom(x1, x2, x3, x4, x5, x6) (-1)
#define getsockopt(x1, x2, x3, x4, x5) (-1)
#define getpeername(x1, x2, x3) (-1)
#define recv(s, b, l, f) (-1)
#define select(x1, x2, x3, x4, x5) (0)

const char *inet_ntop(int af, const void *src, char *dst, int size);

#endif /* CS_PLATFORM == CS_P_STM32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_STM32_H_ */
