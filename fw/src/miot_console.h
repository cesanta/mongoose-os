/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_CONSOLE_H_
#define CS_FW_SRC_MIOT_CONSOLE_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_CONSOLE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void miot_console_init(void);
void miot_console_putc(char c);
void miot_console_printf(const char *fmt, ...);

#if MIOT_ENABLE_RPC
int miot_console_is_waiting_for_resp(void);
#endif

#define CONSOLE_LOG(l, x)          \
  if (cs_log_level >= l) {         \
    cs_log_print_prefix(__func__); \
    cs_log_printf x;               \
    miot_console_printf x;         \
  }

#else

#define CONSOLE_LOG(l, x) LOG(l, x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_CONSOLE */

#endif /* CS_FW_SRC_MIOT_CONSOLE_H_ */
