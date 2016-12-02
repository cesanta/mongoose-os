/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_PIC32_HARMONY_H_
#define CS_COMMON_PLATFORMS_PLATFORM_PIC32_HARMONY_H_

#if CS_PLATFORM == CS_P_PIC32_HARMONY

#define MG_NET_IF MG_NET_IF_PIC32_HARMONY

#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#include <system_config.h>
#include <system_definitions.h>

#include <sys/types.h>

typedef TCP_SOCKET sock_t;
#define to64(x) strtoll(x, NULL, 10)

#define SIZE_T_FMT "lu"
#define INT64_FMT "lld"

char* inet_ntoa(struct in_addr in);

#endif /* CS_PLATFORM == CS_P_PIC32_HARMONY */

#endif /* CS_COMMON_PLATFORMS_PLATFORM_PIC32_HARMONY_H_ */
