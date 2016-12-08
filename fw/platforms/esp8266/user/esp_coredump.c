/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef ESP_COREDUMP

#include <stdio.h>
#include <string.h>
#include <ets_sys.h>
#include <xtensa/corebits.h>
#include <stdint.h>

#include "common/platforms/esp8266/esp_missing_includes.h"

#include "esp_exc.h"
#include "esp_gdb.h"
#include "esp_hw.h"
#include "v7_esp.h"

#include "common/base64.h"

static uint32_t last_char_ts = 0;

static NOINSTR void core_dump_emit_char(char c, void *user_data) {
  int *col_counter = (int *) user_data;
  /* Since we have may have no flow control on dbg uart, limit the speed
   * the we emit the chars at. It's important to deliver core dumps intact. */
  uint32_t now;
  do {
    now = system_get_time();
  } while (now > last_char_ts /* handle overflow */ &&
           now - last_char_ts < 70 /* Char time @ 115200 is 70 us */);
  (*col_counter)++;
  fputc(c, stderr);
  if (*col_counter >= 160) {
    fputc('\n', stderr);
    (*col_counter) = 0;
  }
  last_char_ts = now;
}

/* address must be aligned to 4 and size must be multiple of 4 */
static NOINSTR void emit_core_dump_section(const char *name, uint32_t addr,
                                           uint32_t size) {
  struct cs_base64_ctx ctx;
  int col_counter = 0;
  fprintf(stderr, ", \"%s\": {\"addr\": %u, \"data\": \"", name, addr);
  cs_base64_init(&ctx, core_dump_emit_char, &col_counter);

  uint32_t end = addr + size;
  while (addr < end) {
    uint32_t buf;
    buf = *((uint32_t *) addr);
    addr += sizeof(uint32_t);
    cs_base64_update(&ctx, (char *) &buf, sizeof(uint32_t));
  }
  cs_base64_finish(&ctx);
  fprintf(stderr, "\"}\n");
}

NOINSTR void esp_dump_core(int cause, struct regfile *regs) {
  fprintf(stderr,
          "Dumping core\n"
          "--- BEGIN CORE DUMP ---\n"
          "{\"arch\": \"ESP8266\", \"cause\": %d",
          cause);
  emit_core_dump_section("REGS", (uintptr_t) regs, sizeof(*regs));
  emit_core_dump_section("DRAM", 0x3FFE8000, 0x18000);
  emit_core_dump_section("IRAM", 0x40100000, 0x8000);
  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging anyway
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */
  fprintf(stderr, "}\n---- END CORE DUMP ----\n");
}

#endif /* ESP_COREDUMP */
