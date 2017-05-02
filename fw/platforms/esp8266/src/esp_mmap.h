/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_MMAP_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_MMAP_H_

#include <stdint.h>

#include "common/spiffs/spiffs.h"

#ifdef CS_MMAP

#ifndef MGOS_MMAP_SLOTS
#define MGOS_MMAP_SLOTS 16
#endif

#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)
#define MMAP_DESC_BITS 24
#define FLASH_BASE 0x40200000

#define MMAP_DESC_FROM_ADDR(addr) \
  (&mmap_descs[(((uintptr_t) addr) >> MMAP_DESC_BITS) & 0xF])
#define MMAP_ADDR_FROM_DESC(desc) \
  ((void *) ((uintptr_t) MMAP_BASE | ((desc - mmap_descs) << MMAP_DESC_BITS)))

struct mmap_desc {
  void *base;
  uint32_t pages;
  uint32_t *blocks; /* pages long */
};

extern struct mmap_desc mmap_descs[MGOS_MMAP_SLOTS];

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2);
int esp_spiffs_dummy_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst);
#endif /* CS_MMAP */
#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_MMAP_H_ */
