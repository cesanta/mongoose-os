/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdio.h>

#include "user_interface.h"

#include "esp_hw.h"
#include "esp_fs.h"

IRAM NOINSTR uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;

#ifdef CS_MMAP
  if (addr >= (uint8_t *) MMAP_BASE && addr < (uint8_t *) MMAP_END) {
    struct mmap_desc *desc = MMAP_DESC_FROM_ADDR(addr);
    int block = ((uintptr_t) addr & 0xFFFFFF) / SPIFFS_PAGE_DATA_SIZE;
    int off = ((uintptr_t) addr & 0xFFFFFF) % SPIFFS_PAGE_DATA_SIZE;
    void *ea = (uint8_t *) desc->blocks[block] + off;
    if (ea < (void *) FLASH_BASE) {
      printf("MMAP invalid address %p: block %d, off %d, pages %d, desc %d\n",
             ea, block, off, desc->pages, desc - mmap_descs);
      *(int *) 1 = 1;
    }
    return read_unaligned_byte(ea);
  }
#endif

  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}
