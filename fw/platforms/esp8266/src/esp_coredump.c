/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
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
