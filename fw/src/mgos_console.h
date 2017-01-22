/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_CONSOLE_H_
#define CS_FW_SRC_MGOS_CONSOLE_H_

#include "fw/src/mgos_features.h"

#include "common/cs_dbg.h"

#if MGOS_ENABLE_CONSOLE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void mgos_console_init(void);
void mgos_console_putc(char c);
void mgos_console_printf(const char *fmt, ...);

#if MGOS_ENABLE_RPC
int mgos_console_is_waiting_for_resp(void);
#endif

#define CONSOLE_LOG(l, x)          \
  if (cs_log_level >= l) {         \
    cs_log_print_prefix(__func__); \
    cs_log_printf x;               \
    mgos_console_printf x;         \
  }

#else

#define CONSOLE_LOG(l, x) LOG(l, x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_CONSOLE */

#endif /* CS_FW_SRC_MGOS_CONSOLE_H_ */
