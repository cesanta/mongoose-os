/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a generic implementation of SPIFFS mmapping on esp8266 and esp32.
 *
 * Each of these two archs has arch-specific header which fills a few gaps,
 * see e.g. fw/platforms/esp32/src/esp32_mmap.h.
 *
 * It already works, but there are a few major TODO-s:
 *
 * - Use linked list instead of fixed-size array of mmap descriptors
 * - On esp32, implement reading unaligned bytes from flash in a generic
 *   manner, and rename read_unaligned_byte() below to read_mmapped_byte()
 * - On esp32, use spi_flash_mmap() to map flash area at some address, and
 *   get rid of FLASH_READ_BYTE() (see fw/platforms/esp32/src/esp32_mmap.h)
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common/platforms/esp/src/esp_mmap.h"

#include "common/cs_dbg.h"
#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"

#ifdef CS_MMAP

struct mmap_desc mmap_descs[MGOS_MMAP_SLOTS];
static struct mmap_desc *cur_mmap_desc;

/*
 * Reads unaligned byte, handles spiffs-mmapped addresses properly.
 *
 * TODO(dfrank): handle reading unaligned bytes on esp32 in a generic way,
 * and rename this function to read_mmapped_byte().
 */
IRAM NOINSTR static uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;

  if (addr >= (uint8_t *) MMAP_BASE && addr < (uint8_t *) MMAP_END) {
    struct mmap_desc *desc = MMAP_DESC_FROM_ADDR(addr);
    int block = ((uintptr_t) addr & 0xFFFFFF) / SPIFFS_PAGE_DATA_SIZE;
    int off = ((uintptr_t) addr & 0xFFFFFF) % SPIFFS_PAGE_DATA_SIZE;
    uint8_t *ea = (uint8_t *) desc->blocks[block] + off;

    /*
     * This is commented because on esp32 we might have more than one
     * mount points
     *
     * TODO(dfrank): somehow deduce that from the address, and uncomment
     */
#if 0
    if (ea < (uint8_t *) FLASH_BASE(cs_spiffs_get_fs())) {
      printf("MMAP invalid address %p: block %d, off %d, pages %d, desc %d\n",
             ea, block, off, desc->pages, desc - mmap_descs);
      *(int *) 1 = 1;
    }
#endif

    return FLASH_READ_BYTE(ea);
  }

  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2) {
  int pc_inc = 0;

  uint32_t instr = (uint32_t) read_unaligned_byte(pc) | ((uint32_t) read_unaligned_byte(pc + 1) << 8);
  uint8_t at = (instr >> 4) & 0xf;

  uint32_t val = 0;

  if ((instr & 0xf00f) == 0x2) {
    /* l8ui at, as, imm       r = 0 */
    val = read_unaligned_byte((uint8_t *) vaddr);
    pc_inc = 3;
  } else if ((instr & 0x700f) == 0x1002) {
    /*
     * l16ui at, as, imm      r = 1
     * l16si at, as, imm      r = 9
     */
    val = read_unaligned_byte((uint8_t *) vaddr) |
          read_unaligned_byte((uint8_t *) vaddr + 1) << 8;
    if (instr & 0x8000) val = (int16_t) val;
    pc_inc = 3;
  } else if ((instr & 0xf00f) == 0x2002 || (instr & 0xf) == 0x8) {
    /*
     * l32i   at, as, imm      r = 2
     * l32i.n at, as, imm
     */
    /*
     * TODO(dfrank): provide fast code path for aligned access since
     * all mmap 32-bit loads will be aligned.
     */
    val = read_unaligned_byte((uint8_t *) vaddr) |
          read_unaligned_byte((uint8_t *) vaddr + 1) << 8 |
          read_unaligned_byte((uint8_t *) vaddr + 2) << 16 |
          read_unaligned_byte((uint8_t *) vaddr + 3) << 24;
    pc_inc = ((instr & 0xf) == 0x8) ? 2 : 3;
  } else {
    fprintf(stderr, "cannot emulate flash mem instr at pc = %p\n", (void *) pc);
  }

  if (pc_inc > 0) {
    /*
     * a0 and a1 are never used as scratch registers.
     * Here we assume that a2...15 are laid out contiguously in the struct.
     */
    *(pa2 + at - 2) = val;
  }

  return pc_inc;
}

static struct mmap_desc *alloc_mmap_desc(void) {
  size_t i;
  for (i = 0; i < sizeof(mmap_descs) / sizeof(mmap_descs[0]); i++) {
    if (mmap_descs[i].blocks == NULL) {
      return &mmap_descs[i];
    }
  }
  return NULL;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
  int pages = (len + SPIFFS_PAGE_DATA_SIZE - 1) / SPIFFS_PAGE_DATA_SIZE;
  struct mmap_desc *desc = alloc_mmap_desc();
  (void) addr;
  (void) prot;
  (void) flags;
  (void) offset;

  if (len == 0) {
    return NULL;
  }

  if (desc == NULL) {
    LOG(LL_ERROR, ("cannot allocate mmap desc"));
    return MAP_FAILED;
  }

  cur_mmap_desc = desc;
  desc->pages = 0;
  desc->blocks = (uint32_t *) calloc(sizeof(uint32_t), pages);
  if (desc->blocks == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return MAP_FAILED;
  }
  desc->base = MMAP_ADDR_FROM_DESC(desc);

  {
    spiffs *fs;
    fd = esp_translate_fd(fd, &fs);
    if (fd < 0) {
      LOG(LL_ERROR, ("Failed to translate fd: %d", fd));
      return NULL;
    }

    int32_t t;
    t = SPIFFS_read(fs, fd, DUMMY_MMAP_BUFFER_START, len);
    if (t != (int32_t)len) {
      LOG(LL_ERROR, ("Spiffs dummy read failed: expected len: %d, actual: %d",
            len, t));
      return NULL;
    }
    /*
     * this breaks the posix-like mmap abstraction but file descriptors are a
     * scarse resource here.
     */
    t = SPIFFS_close(fs, fd);
    if (t != 0) {
      LOG(LL_ERROR, ("Failed to close descr after dummy read: %d", t));
      return NULL;
    }
  }

  return desc->base;
}

/*
 * Relocate mmapped pages.
 */
void esp_spiffs_on_page_move_hook(spiffs *fs, spiffs_file fh,
                                  spiffs_page_ix src_pix,
                                  spiffs_page_ix dst_pix) {
  size_t i, j;
  (void) fh;
  uint32_t fbase = FLASH_BASE(fs);
  for (i = 0; i < ARRAY_SIZE(mmap_descs); i++) {
    if (mmap_descs[i].blocks) {
      for (j = 0; j < mmap_descs[i].pages; j++) {
        uint32_t addr = mmap_descs[i].blocks[j];
        uint32_t page = SPIFFS_PADDR_TO_PAGE(fs, addr - fbase);
        if (page == src_pix) {
          int delta = (int) dst_pix - (int) src_pix;
          mmap_descs[i].blocks[j] += delta * LOG_PAGE_SIZE;
        }
      }
    }
  }
}

int esp_spiffs_dummy_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  (void) fs;
  (void) size;

  if (dst >= DUMMY_MMAP_BUFFER_START && dst < DUMMY_MMAP_BUFFER_END) {
    if ((addr - SPIFFS_PAGE_HEADER_SIZE) % LOG_PAGE_SIZE == 0) {
      addr &= 0xFFFFF;
      cur_mmap_desc->blocks[cur_mmap_desc->pages++] = FLASH_BASE(fs) + addr;
    }
    return 1;
  }

  return 0;
}
#endif /* CS_MMAP */
