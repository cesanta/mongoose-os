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

#include <stdint.h>
#include <string.h>

#include <user_interface.h>
#include <xtensa/corebits.h>

#include "common/cs_base64.h"
#include "common/cs_crc32.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "esp_exc.h"

#include "mgos_core_dump.h"

inline void mgos_cd_putc(int c) {
  esp_exc_putc(c);
}

NOINSTR void esp_dump_core(uint32_t cause, struct regfile *regs) {
  (void) cause;
  mgos_cd_emit_header();
  mgos_cd_emit_section(MGOS_CORE_DUMP_SECTION_REGS, regs, sizeof(*regs));
  mgos_cd_emit_section("DRAM", (void *) 0x3FFE8000, 0x18000);
  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging anyway
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */
  mgos_cd_emit_footer();
}
