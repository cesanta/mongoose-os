/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#define CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#if CS_PLATFORM == CS_P_ESP8266

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <machine/endian.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl
#define _FILE_OFFSET_BITS 32

#define MG_LWIP 1

#ifdef RTOS_SDK
#  define MG_NET_IF MG_NET_IF_SOCKET
#else
#  define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL
#endif

/* struct timeval is defined in sys/time.h. */
#define LWIP_TIMEVAL_PRIVATE 0

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#endif /* CS_PLATFORM == CS_P_ESP8266 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_ */
