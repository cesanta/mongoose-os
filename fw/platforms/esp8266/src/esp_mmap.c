/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <sys/mman.h>

#include "common/cs_dbg.h"
#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"

#include "fw/platforms/esp8266/src/esp_fs.h"
#include "fw/platforms/esp8266/src/esp_mmap.h"

#ifdef CS_MMAP

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

struct mmap_desc mmap_descs[MGOS_MMAP_SLOTS];
static struct mmap_desc *cur_mmap_desc;

IRAM NOINSTR static uint8_t read_mmapped_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;

  if (addr >= (uint8_t *) MMAP_BASE && addr < (uint8_t *) MMAP_END) {
    struct mmap_desc *desc = MMAP_DESC_FROM_ADDR(addr);
    int block = ((uintptr_t) addr & 0xFFFFFF) / SPIFFS_PAGE_DATA_SIZE;
    int off = ((uintptr_t) addr & 0xFFFFFF) % SPIFFS_PAGE_DATA_SIZE;
    uint8_t *ea = (uint8_t *) desc->blocks[block] + off;
    if (ea < (uint8_t *) FLASH_BASE) {
      printf("MMAP invalid address %p: block %d, off %d, pages %d, desc %d\n",
             ea, block, off, desc->pages, desc - mmap_descs);
      *(int *) 1 = 1;
    }
    return *ea;
  }

  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2) {
  int pc_inc = 0;

  uint32_t instr = (uint32_t) *pc | ((uint32_t) * (pc + 1) << 8);
  uint8_t at = (instr >> 4) & 0xf;

  uint32_t val = 0;

  if ((instr & 0xf00f) == 0x2) {
    /* l8ui at, as, imm       r = 0 */
    val = read_mmapped_byte((uint8_t *) vaddr);
    pc_inc = 3;
  } else if ((instr & 0x700f) == 0x1002) {
    /*
     * l16ui at, as, imm      r = 1
     * l16si at, as, imm      r = 9
     */
    val = read_mmapped_byte((uint8_t *) vaddr) |
          read_mmapped_byte((uint8_t *) vaddr + 1) << 8;
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
    val = read_mmapped_byte((uint8_t *) vaddr) |
          read_mmapped_byte((uint8_t *) vaddr + 1) << 8 |
          read_mmapped_byte((uint8_t *) vaddr + 2) << 16 |
          read_mmapped_byte((uint8_t *) vaddr + 3) << 24;
    // esp_exc_puts("==a1\r\n");
    pc_inc = ((instr & 0xf) == 0x8) ? 2 : 3;
  } else {
    fprintf(stderr, "cannot emulate flash mem instr at pc = %p\n", (void *) pc);
  }

  // fprintf(stderr, "hey at=%d\n", at);

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
  SPIFFS_read(cs_spiffs_get_fs(), fd - NUM_SYS_FD, DUMMY_MMAP_BUFFER_START,
              len);
  /*
   * this breaks the posix-like mmap abstraction but file descriptors are a
   * scarse resource here.
   */
  SPIFFS_close(cs_spiffs_get_fs(), fd - NUM_SYS_FD);

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
  /* for (i = 0; i < (sizeof(mmap_descs) / sizeof(mmap_descs[0])); i++) { */
  for (i = 0; i < ARRAY_SIZE(mmap_descs); i++) {
    if (mmap_descs[i].blocks) {
      for (j = 0; j < mmap_descs[i].pages; j++) {
        uint32_t addr = mmap_descs[i].blocks[j];
        uint32_t page = SPIFFS_PADDR_TO_PAGE(fs, addr - FLASH_BASE);
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
      /*
       * If FW uses OTA (and flash mapping) addr might be > 0x100000
       * and FLASH_BASE + addr will point somewhere behind flash
       * mapped area (40200000h-40300000h)
       * So, we need map it back.
       * (i.e. if addr > 0x100000 -> addr -= 0x100000)
       */
      addr &= 0xFFFFF;
      cur_mmap_desc->blocks[cur_mmap_desc->pages++] = FLASH_BASE + addr;
    }
    return 1;
  }

  return 0;
}
#endif /* CS_MMAP */
