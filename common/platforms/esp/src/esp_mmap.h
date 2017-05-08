/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_ESP_SRC_ESP_MMAP_H_
#define CS_COMMON_PLATFORMS_ESP_SRC_ESP_MMAP_H_

#include <stdint.h>

#include "common/spiffs/spiffs.h"
#include "common/platform.h"

/*
 * Platform-dependent header should define the following macros:
 *
 * - MMAP_BASE: base address for mmapped points; e.g. ((void *) 0x10000000)
 * - MMAP_END:  end address for mmapped points; e.g. ((void *) 0x20000000)
 *
 * So with the example values given above, the range 0x10000000 - 0x20000000 is
 * used for all mmapped areas. We need to partition it further, by choosing the
 * optimal tradeoff between the max number of mmapped areas and the max size
 * of the mmapped area. Within the example range, we have 28 bits, and we
 * need to define two more macros which will define how these bits are used:
 *
 * - MMAP_ADDR_BITS: how many bits are used for the address within each
 *   mmapped area;
 * - MMAP_NUM_BITS: how many bits are used for the number of mmapped area.
 */

#if CS_PLATFORM == CS_P_ESP32
#include "fw/platforms/esp32/src/esp32_fs.h"
#include "fw/platforms/esp32/src/esp32_mmap.h"
#elif CS_PLATFORM == CS_P_ESP8266
#include "fw/platforms/esp8266/src/esp8266_mmap.h"
#include "fw/platforms/esp8266/src/esp_fs.h"
#else
#error unsupported CS_PLATFORM: only esp32 and esp8266 are supported
#endif

#ifdef CS_MMAP

#define MMAP_NUM_MASK ((1 << MMAP_NUM_BITS) - 1)

#define MMAP_DESC_FROM_ADDR(addr) \
  (&mmap_descs[(((uintptr_t) addr) >> MMAP_ADDR_BITS) & MMAP_NUM_MASK])
#define MMAP_ADDR_FROM_DESC(desc) \
  ((void *) ((uintptr_t) MMAP_BASE | ((desc - mmap_descs) << MMAP_ADDR_BITS)))

struct mmap_desc {
  void *base;
  uint32_t pages;
  uint32_t *blocks; /* pages long */
};

extern struct mmap_desc *mmap_descs;

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2);
int esp_spiffs_dummy_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst);

/*
 * Arch-dependent: translates vfs file descriptor from vfs to a spiffs file
 * descriptor; also returns an instance of spiffs through the pointer.
 */
int esp_translate_fd(int fd, spiffs **pfs);

void esp_mmap_init(void);

#endif /* CS_MMAP */
#endif /* CS_COMMON_PLATFORMS_ESP_SRC_ESP_MMAP_H_ */
