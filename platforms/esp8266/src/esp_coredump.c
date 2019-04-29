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

inline void mgos_cd_putc(int c) {
  esp_exc_putc(c);
}

static struct regfile *s_regs;

void esp_dump_core(uint32_t cause, struct regfile *regs) {
  s_regs = regs;
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
  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging anyway
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */
}
