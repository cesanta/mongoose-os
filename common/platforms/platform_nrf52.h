/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#define CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#if CS_PLATFORM == CS_P_NRF52

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MG_NET_IF             MG_NET_IF_LWIP_LOW_LEVEL
#define LWIP_TIMEVAL_PRIVATE  0
#define LWIP_PROVIDE_ERRNO    1
#define MG_LWIP               1
#define MG_ENABLE_IPV6        1

#define INT64_FMT PRId64
#define SIZE_T_FMT "u"

#endif /* CS_PLATFORM == CS_P_NRF52 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_ */
