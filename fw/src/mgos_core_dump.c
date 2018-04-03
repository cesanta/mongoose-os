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

#include "mgos_core_dump.h"

#include <stdint.h>
#include <string.h>

#include "common/cs_base64.h"
#include "common/cs_crc32.h"
#include "common/str_util.h"

#include "mgos_system.h"

#ifndef NOINSTR
#define NOINSTR
#endif

void mgos_cd_puts(const char *s) {
  while (*s != '\0') mgos_cd_putc(*s++);
}

void mgos_cd_printf(const char *fmt, ...) {
  va_list ap;
  char buf[100];
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  mgos_cd_puts(buf);
}

struct section_ctx {
  struct cs_base64_ctx b64_ctx;
  int col_counter;
  uint32_t crc32;
};

static NOINSTR void emit_char(char c, void *user_data) {
  struct section_ctx *ctx = (struct section_ctx *) user_data;
  mgos_cd_putc(c);
  ctx->col_counter++;
  if (ctx->col_counter >= 160) {
    mgos_cd_puts("\r\n");
    ctx->col_counter = 0;
    mgos_wdt_feed();
  }
}

NOINSTR void mgos_cd_emit_section(const char *name, const void *p, size_t len) {
  char buf[100];
  struct section_ctx ctx = {.col_counter = 0, .crc32 = 0};
  cs_base64_init(&ctx.b64_ctx, emit_char, &ctx);
  sprintf(buf, ",\r\n\"%s\": {\"addr\": %u, \"data\": \"\r\n", name,
          (unsigned int) p);
  mgos_cd_puts(buf);

  const uint32_t *dp = (const uint32_t *) p;
  const uint32_t *end = dp + (len / sizeof(uint32_t));
  while (dp < end) {
    uint32_t tmp = *dp++;
    ctx.crc32 = cs_crc32(ctx.crc32, &tmp, sizeof(tmp));
    cs_base64_update(&ctx.b64_ctx, (char *) &tmp, sizeof(tmp));
  }
  cs_base64_finish(&ctx.b64_ctx);
  sprintf(buf, "\", \"crc32\": %u}", (unsigned int) ctx.crc32);
  mgos_cd_puts(buf);
}

void mgos_cd_emit_header(void) {
  mgos_cd_puts(
      "\r\n--- BEGIN CORE DUMP ---\r\n{\"arch\": \"" CS_STRINGIFY_MACRO(
          FW_ARCHITECTURE) "\"");
}

NOINSTR void mgos_cd_emit_footer(void) {
  mgos_cd_puts("}\r\n---- END CORE DUMP ----\r\n");
}
