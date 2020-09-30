/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_CORE_DUMP_SECTION_REGS "REGS"
#define MGOS_CORE_DUMP_BEGIN "--- BEGIN CORE DUMP ---"
#define MGOS_CORE_DUMP_END "---- END CORE DUMP ----"

typedef void (*mgos_cd_section_writer_f)(void);
void mgos_cd_register_section_writer(mgos_cd_section_writer_f writer);

void mgos_cd_write(void);
void mgos_cd_write_section(const char *name, const void *p, size_t len);

#ifndef MGOS_BOOT_BUILD
void mgos_cd_puts(const char *s);
void mgos_cd_printf(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
/* Platform must provide this. It must be safe to invoke from an ISR. */
extern void mgos_cd_putc(int c);
#else
#include "mgos_boot_dbg.h"
#define mgos_cd_printf mgos_boot_dbg_printf
#define mgos_cd_puts mgos_boot_dbg_puts
#define mgos_cd_putc mgos_boot_dbg_putc
#endif /* MGOS_BOOT_BUILD */

#ifdef __cplusplus
}
#endif
