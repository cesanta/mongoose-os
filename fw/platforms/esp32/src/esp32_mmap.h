/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_MMAP_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_MMAP_H_

#ifdef CS_MMAP

#include <esp_vfs.h>

#include "fw/platforms/esp32/src/esp32_fs.h"

/*
 * See common/platforms/esp/src/esp_mmap.h for details of these values
 */

#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)

#define MMAP_ADDR_BITS 20
#define MMAP_NUM_BITS 8

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

/*
 * TODO(dfrank): mmap needed flash area with spi_flash_mmap(), and just
 * dereference the addr (and probably even remove this FLASH_READ_BYTE()
 * abstraction, since on esp32 it also just dereferences the addr)
 */
#define FLASH_READ_BYTE(addr)                            \
  ({                                                     \
    uint8_t tmp;                                         \
    spi_flash_read((uint32_t)(addr), &tmp, sizeof(tmp)); \
    tmp;                                                 \
  })

#define FLASH_BASE(fs) \
  (((struct mount_info *) ((fs)->user_data))->part->address)

#endif /* CS_MMAP */
#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_MMAP_H_ */
