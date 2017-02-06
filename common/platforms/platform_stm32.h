/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#if CS_PLATFORM == CS_P_STM32

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <stm32_sdk_hal.h>

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 1
#endif

#define CS_DEFINE_DIRENT

#endif /* CS_PLATFORM == CS_P_STM32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_STM32_H_ */
