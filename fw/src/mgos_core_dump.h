/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_CORE_DUMP_H_
#define CS_FW_SRC_MGOS_CORE_DUMP_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_CORE_DUMP_SECTION_REGS "REGS"

void mgos_cd_emit_header(void);
void mgos_cd_emit_section(const char *name, const void *p, size_t len);
void mgos_cd_emit_footer(void);

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

#endif /* CS_FW_SRC_MGOS_CORE_DUMP_H_ */
