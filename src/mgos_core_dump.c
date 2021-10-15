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

#include <stdlib.h>
#include <string.h>

#include "common/cs_base64.h"
#include "common/cs_crc32.h"
#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "mgos_system.h"
#include "mgos_time.h"

#ifndef NOINSTR
#define NOINSTR
#endif

extern const char *build_version, *build_id, *build_timestamp;

static mgos_cd_section_writer_f s_section_writers[8];

#ifndef MGOS_BOOT_BUILD
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
#endif  // MGOS_BOOT_BUILD

struct section_ctx {
  struct cs_base64_ctx b64_ctx;
  int col_counter;
  uint32_t crc32;
};

static NOINSTR void write_char(char c, void *user_data) {
  struct section_ctx *ctx = (struct section_ctx *) user_data;
  mgos_cd_putc(c);
  ctx->col_counter++;
  if (ctx->col_counter >= 160) {
    mgos_cd_puts("\n");
    ctx->col_counter = 0;
    mgos_wdt_feed();
  }
}

NOINSTR void mgos_cd_write_section(const char *name, const void *p,
                                   size_t len) {
  struct section_ctx ctx = {.col_counter = 0, .crc32 = 0};
  cs_base64_init(&ctx.b64_ctx, write_char, &ctx);
  mgos_cd_printf(",\n\"%s\": {\"addr\": %lu, \"data\": \"\n", name,
                 (unsigned long) p);
  mgos_wdt_feed();
  const uint32_t *dp = (const uint32_t *) p;
  const uint32_t *end = dp + (len / sizeof(uint32_t));
  while (dp < end) {
    uint32_t tmp = *dp++;
    ctx.crc32 = cs_crc32(ctx.crc32, &tmp, sizeof(tmp));
    cs_base64_update(&ctx.b64_ctx, (char *) &tmp, sizeof(tmp));
  }
  cs_base64_finish(&ctx.b64_ctx);
  mgos_cd_printf("\", \"crc32\": %u}", (unsigned int) ctx.crc32);
}

extern enum cs_log_level cs_log_level;

NOINSTR void mgos_cd_write(void) {
  cs_log_level = LL_NONE;
  mgos_cd_puts("\n" MGOS_CORE_DUMP_BEGIN "\n{");
  mgos_cd_puts("\"app\": \"" MGOS_APP "\", ");
  mgos_cd_puts("\"arch\": \"" CS_STRINGIFY_MACRO(FW_ARCHITECTURE) "\", ");
  mgos_cd_printf("\"version\": \"%s\", ", build_version);
  mgos_cd_printf("\"build_id\": \"%.80s\", ", build_id);
  mgos_cd_printf("\"build_ts\": \"%s\",\n", build_timestamp);
#ifdef MGOS_SDK_BUILD_IMAGE
  mgos_cd_printf("\"build_image\": \"" MGOS_SDK_BUILD_IMAGE "\", ");
#endif
  mgos_cd_printf("\"uptime\": %lld", (long long) mgos_uptime_micros());
// For ARM targets, add profile information.
#if defined(__FPU_PRESENT)
#if __FPU_PRESENT
  mgos_cd_printf(", \"target_features\": \"arm-with-m-vfp-d16.xml\"");
#else
  mgos_cd_printf(", \"target_features\": \"arm-with-m.xml\"");
#endif
#endif

  for (int i = 0; i < (int) ARRAY_SIZE(s_section_writers); i++) {
    if (s_section_writers[i] == NULL) break;
    s_section_writers[i]();
  }

  mgos_cd_puts("}\n" MGOS_CORE_DUMP_END "\n");
}

void mgos_cd_register_section_writer(mgos_cd_section_writer_f writer) {
  for (int i = 0; i < (int) ARRAY_SIZE(s_section_writers); i++) {
    if (s_section_writers[i] == NULL) {
      s_section_writers[i] = writer;
      break;
    }
  }
}
