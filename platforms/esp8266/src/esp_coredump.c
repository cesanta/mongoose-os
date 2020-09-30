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

#include "esp_coredump.h"

#include <stdint.h>
#include <string.h>

#include <user_interface.h>
#include <xtensa/corebits.h>

#include "common/cs_base64.h"
#include "common/cs_crc32.h"
#include "esp_missing_includes.h"

#include "mgos_core_dump.h"

#include "esp_flash_writer.h"

// We mask the address and length to enure it's not been
// overwritten with random values due to corruption.
#define FLASH_ADDR_MAGIC 0xab000000
#define FLASH_SIZE_MAGIC 0xcd000000
#define FLASH_ADDR_MAGIC_MASK 0xff000000
#define FLASH_SIZE_MAGIC_MASK 0xff000000

static struct regfile *s_regs;
static struct esp_flash_write_ctx s_cd_write_ctx;

void mgos_cd_putc(int c) {
  esp_exc_putc(c);
  if (s_cd_write_ctx.addr != 0 &&
      (s_cd_write_ctx.addr & FLASH_ADDR_MAGIC_MASK) == 0) {
    esp_flash_write(&s_cd_write_ctx, mg_mk_str_n((char *) &c, 1));
  }
}

void esp_core_dump_set_flash_area(uint32_t addr, uint32_t max_size) {
  s_cd_write_ctx.addr = addr | FLASH_ADDR_MAGIC;
  s_cd_write_ctx.max_size = max_size | FLASH_SIZE_MAGIC;
}

void esp_dump_core(uint32_t cause, struct regfile *regs) {
  s_regs = regs;
  // Check flash write settings.
  if (((s_cd_write_ctx.addr & FLASH_ADDR_MAGIC_MASK) != FLASH_ADDR_MAGIC) ||
      ((s_cd_write_ctx.max_size & FLASH_SIZE_MAGIC_MASK) != FLASH_SIZE_MAGIC)) {
    // Write context is invalid, wipe it to prevent flash writes from now on.
    memset(&s_cd_write_ctx, 0, sizeof(s_cd_write_ctx));
  } else {
    // Unmask the address and size to enable writes.
    uint32_t cd_addr = (s_cd_write_ctx.addr & ~FLASH_ADDR_MAGIC_MASK);
    uint32_t cd_size = (s_cd_write_ctx.max_size & ~FLASH_SIZE_MAGIC_MASK);
    mgos_cd_printf("Core dump flash area: %lu @ %#lx\n", cd_size, cd_addr);
    esp_init_flash_write_ctx(&s_cd_write_ctx, cd_addr, cd_size);
  }
  mgos_cd_write();
  (void) cause;
}

static void esp_dump_regs(void) {
  mgos_cd_write_section(MGOS_CORE_DUMP_SECTION_REGS, s_regs, sizeof(*s_regs));
}

static void esp_dump_dram(void) {
  mgos_cd_write_section("DRAM", (void *) 0x3FFE8000, 0x18000);
}

void esp_core_dump_init(void) {
  mgos_cd_register_section_writer(esp_dump_regs);
  mgos_cd_register_section_writer(esp_dump_dram);
  esp_init_flash_write_ctx(&s_cd_write_ctx, 0, 0);
  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */
}
