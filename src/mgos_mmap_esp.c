/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CS_MMAP

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mgos_mmap_esp_internal.h"
#include "mgos_vfs.h"

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/platform.h"
#include "spiffs.h"
#include "spiffs_nucleus.h"

#if CS_PLATFORM != CS_P_ESP32 && CS_PLATFORM != CS_P_ESP8266
#error only esp32 and esp8266 are supported
#endif

/*
 * Reads unaligned byte, handles mmapped addresses properly.
 *
 * TODO(dfrank):
 * On esp32, just dereferencing the `uint8_t *` which points to the instruction
 * memory results in a load-store-error, like on esp8266. On esp8266 we have a
 * generic exception handler which emulates that, on esp32 it's TODO.
 *
 * So when esp32 also handles it generically, we need to rename
 * read_unaligned_byte to read_mmapped_byte, and don't use it to read the
 * instruction in esp_mmap_exception_handler
 */
IRAM NOINSTR static uint8_t read_unaligned_byte(uint8_t *addr) {
  uint32_t *base = (uint32_t *) ((uintptr_t) addr & ~0x3);
  uint32_t word;

  if (addr >= (uint8_t *) MMAP_BASE && addr < (uint8_t *) MMAP_END) {
    /* The byte is mmapped */
    struct mgos_vfs_mmap_desc *desc = MMAP_DESC_FROM_ADDR(addr);
    return desc->fs->ops->read_mmapped_byte(desc, MMAP_ADDR_FROM_ADDR(addr));
  }

  word = *base;
  return (uint8_t)(word >> 8 * ((uintptr_t) addr & 0x3));
}

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2) {
  int pc_inc = 0;

  uint32_t instr = (uint32_t) read_unaligned_byte(pc) |
                   ((uint32_t) read_unaligned_byte(pc + 1) << 8);
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

#endif /* CS_MMAP */
