/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_

#include "fw/platforms/esp8266/src/esp_fs.h"

#ifdef CS_MMAP

/*
 * See common/platforms/esp/src/esp_mmap.h for details of these values
 */

#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)

#define MMAP_ADDR_BITS 20
#define MMAP_NUM_BITS 8

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

#define FLASH_READ_BYTE(addr) (*(addr))

#define FLASH_BASE(fs) 0x40200000

#endif /* CS_MMAP */
#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP8266_MMAP_H_ */
