/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#if CS_PLATFORM == CS_P_STM32

#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define SIZE_T_FMT "u"

#endif /* CS_PLATFORM == CS_P_STM32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_STM32_H_ */
