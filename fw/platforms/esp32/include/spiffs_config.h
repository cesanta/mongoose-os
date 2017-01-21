/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_INCLUDE_SPIFFS_CONFIG_H_
#define CS_FW_PLATFORMS_ESP32_INCLUDE_SPIFFS_CONFIG_H_

#if MGOS_ESP32_ENABLE_FLASH_ENCRYPTION
#define SPIFFS_OBJ_NAME_LEN 45
#define SPIFFS_OBJ_META_LEN 8
#define CS_SPIFFS_ENABLE_ENCRYPTION 1
#define CS_SPIFFS_ENCRYPTION_BLOCK_SIZE 32
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

typedef int32_t s32_t;
typedef uint32_t u32_t;
typedef int16_t s16_t;
typedef uint16_t u16_t;
typedef int8_t s8_t;
typedef uint8_t u8_t;

#include "spiffs_config_common.h"

#endif /* CS_FW_PLATFORMS_ESP32_INCLUDE_SPIFFS_CONFIG_H_ */
