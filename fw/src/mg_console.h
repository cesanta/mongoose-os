/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CONSOLE_H_
#define CS_FW_SRC_MG_CONSOLE_H_

void mg_console_init(void);
void mg_console_putc(char c);
void mg_console_printf(const char *fmt, ...);

#ifdef MG_ENABLE_CLUBBY
int mg_console_is_waiting_for_resp(void);

#define CONSOLE_LOG(l, x)          \
  if (cs_log_level >= l) {         \
    cs_log_print_prefix(__func__); \
    cs_log_printf x;               \
    mg_console_printf x;           \
  }
#else
#define CONSOLE_LOG(l, x) LOG(l, x)
#endif /* MG_ENABLE_CLUBBY */

#endif /* CS_FW_SRC_MG_CONSOLE_H_ */
