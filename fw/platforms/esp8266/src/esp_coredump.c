/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef ESP_COREDUMP

#include <stdint.h>
#include <string.h>

#include <user_interface.h>
#include <xtensa/corebits.h>

#include "common/cs_base64.h"
#include "common/cs_crc32.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "esp_exc.h"

struct section_ctx {
  struct cs_base64_ctx b64_ctx;
  int col_counter;
  uint32_t crc32;
};

static NOINSTR void core_dump_emit_char(char c, void *user_data) {
  struct section_ctx *ctx = (struct section_ctx *) user_data;
  esp_exc_putc(c);
  ctx->col_counter++;
  if (ctx->col_counter >= 160) {
    esp_exc_puts("\r\n");
    ctx->col_counter = 0;
  }
}

/* address must be aligned to 4 and size must be multiple of 4 */
static NOINSTR void emit_core_dump_section(const char *name, uint32_t addr,
                                           uint32_t size) {
  struct section_ctx ctx = {.col_counter = 0, .crc32 = 0};
  cs_base64_init(&ctx.b64_ctx, core_dump_emit_char, &ctx);
  esp_exc_printf(",\r\n\"%s\": {\"addr\": %u, \"data\": \"\r\n", name, addr);

  uint32_t end = addr + size;
  while (addr < end) {
    uint32_t tmp;
    tmp = *((uint32_t *) addr);
    addr += sizeof(tmp);
    ctx.crc32 = cs_crc32(ctx.crc32, &tmp, sizeof(tmp));
    cs_base64_update(&ctx.b64_ctx, (char *) &tmp, sizeof(tmp));
  }
  cs_base64_finish(&ctx.b64_ctx);
  esp_exc_printf("\", \"crc32\": %u}", ctx.crc32);
}

NOINSTR void esp_dump_core(uint32_t cause, struct regfile *regs) {
  (void) cause;
  esp_exc_puts("\r\n--- BEGIN CORE DUMP ---\r\n{\"arch\": \"ESP8266\"");
  emit_core_dump_section("REGS", (uintptr_t) regs, sizeof(*regs));
  emit_core_dump_section("DRAM", 0x3FFE8000, 0x18000);
  emit_core_dump_section("IRAM", 0x40100000, 0x8000);
  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging anyway
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */
  esp_exc_puts("}\r\n---- END CORE DUMP ----\r\n");
}

#endif /* ESP_COREDUMP */
