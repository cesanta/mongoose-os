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

void mgos_cd_puts(const char *s);
void mgos_cd_printf(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
/* Platform must provide this. It must be safe to invoke from an ISR. */
extern void mgos_cd_putc(int c);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_CORE_DUMP_H_ */
